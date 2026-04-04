#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "bsp_espat.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

extern UART_HandleTypeDef huart2;
extern void esp_lock(void);
extern void esp_unlock(void);

#define ESP_UART_HANDLE (&huart2)

typedef enum
{
    AT_ACK_NONE,
    AT_ACK_OK,
    AT_ACK_ERROR,
    AT_ACK_BUSY,
    AT_ACK_READY,
} at_ack_t;

typedef struct
{
    at_ack_t ack;
    const char *string;
} at_ack_match_t;

static const at_ack_match_t at_ack_matches[] = 
{
    {AT_ACK_OK, "OK\r\n"},
    {AT_ACK_ERROR, "ERROR\r\n"},
    {AT_ACK_BUSY, "busy p..."},
    {AT_ACK_READY, "ready\r\n"},
};

static char rxbuf[2048];

static void esp_at_usart_write(const char *data);

static at_ack_t esp_at_usart_wait_receive(uint32_t timeout)
{
    uint32_t rxlen = 0;
    uint32_t start = HAL_GetTick();
    
    memset(rxbuf, 0, sizeof(rxbuf));
    
    while (rxlen < sizeof(rxbuf) - 1)
    {
        if(__HAL_UART_GET_FLAG(ESP_UART_HANDLE, UART_FLAG_ORE))
        {
            __HAL_UART_CLEAR_OREFLAG(ESP_UART_HANDLE);
            (void)ESP_UART_HANDLE->Instance->DR; 
        }

        if (__HAL_UART_GET_FLAG(ESP_UART_HANDLE, UART_FLAG_RXNE) == SET)
        {
            uint8_t data = (uint8_t)(ESP_UART_HANDLE->Instance->DR & 0xFF);
            rxbuf[rxlen++] = data;
            
            if (rxbuf[rxlen - 1] == '\n')
            {
                for (uint32_t i = 0; i < ARRAY_SIZE(at_ack_matches); i++)
                {
                    if (strstr(rxbuf, at_ack_matches[i].string) != NULL)
                    {
                        return at_ack_matches[i].ack;
                    }
                }
            }
        }
        else
        {
            if ((HAL_GetTick() - start) >= timeout)
            {
                return AT_ACK_NONE;
            }
        }
    }
    
    return AT_ACK_NONE;
}

static void esp_at_usart_write(const char *data)
{
    if (data && *data)
    {
        HAL_UART_Transmit(ESP_UART_HANDLE, (uint8_t*)data, strlen(data), 1000);
    }
    
    uint8_t newline[] = {'\r', '\n'};
    HAL_UART_Transmit(ESP_UART_HANDLE, newline, 2, 100);
}

bool esp_at_wait_ready(uint32_t timeout)
{
    return esp_at_usart_wait_receive(timeout) == AT_ACK_READY;
}

bool esp_at_write_command(const char *command, uint32_t timeout)
{
    esp_at_usart_write(command);
    at_ack_t ack = esp_at_usart_wait_receive(timeout);
    return ack == AT_ACK_OK;
}

bool esp_at_write_command_locked(const char *command, uint32_t timeout)
{
    esp_lock();
    bool result = esp_at_write_command(command, timeout);
    esp_unlock();
    return result;
}

const char *esp_at_get_response(void)
{
    return rxbuf;
}

bool esp_at_init(void)
{
    uint32_t start;
    uint32_t rxlen;

    __HAL_UART_CLEAR_OREFLAG(ESP_UART_HANDLE);
    __HAL_UART_CLEAR_NEFLAG(ESP_UART_HANDLE);
    __HAL_UART_CLEAR_FEFLAG(ESP_UART_HANDLE);

    HAL_Delay(5000);

    const uint32_t baud_rates[] = {115200};
    
    for (int i = 0; i < 1; i++)
    {
        huart2.Init.BaudRate = baud_rates[i];
        if (HAL_UART_Init(&huart2) != HAL_OK)
        {
            continue;
        }
        
        memset(rxbuf, 0, sizeof(rxbuf));
        start = HAL_GetTick();
        while ((HAL_GetTick() - start) < 1000)
        {
            if (__HAL_UART_GET_FLAG(ESP_UART_HANDLE, UART_FLAG_RXNE) == SET)
            {
                (void)ESP_UART_HANDLE->Instance->DR;
            }
        }
        
        esp_at_usart_write("AT");
        
        start = HAL_GetTick();
        rxlen = 0;
        
        while ((HAL_GetTick() - start) < 3000 && rxlen < sizeof(rxbuf) - 1)
        {
            if (__HAL_UART_GET_FLAG(ESP_UART_HANDLE, UART_FLAG_RXNE) == SET)
            {
                uint8_t data = (uint8_t)(ESP_UART_HANDLE->Instance->DR & 0xFF);
                if (rxlen < sizeof(rxbuf) - 1) {
                    rxbuf[rxlen++] = data;
                }
            }
        }
        
        if (strstr((char*)rxbuf, "OK") != NULL)
        {
            goto baud_found;
        }
        
        if (strstr((char*)rxbuf, "ERROR") != NULL)
        {
            goto baud_found;
        }
    }
    
    return false;
    
baud_found:
    esp_at_usart_write("AT+RST");
    HAL_Delay(5000);
    
    memset(rxbuf, 0, sizeof(rxbuf));
    start = HAL_GetTick();
    while ((HAL_GetTick() - start) < 3000)
    {
        if (__HAL_UART_GET_FLAG(ESP_UART_HANDLE, UART_FLAG_RXNE) == SET)
        {
            (void)ESP_UART_HANDLE->Instance->DR;
        }
    }
    
    for (int retry = 0; retry < 10; retry++)
    {
        if (esp_at_write_command("AT", 3000))
        {
            goto at_ok;
        }
        HAL_Delay(2000);
    }
    return false;
    
at_ok:
    esp_at_write_command("ATE0", 500);
    
    return true;
}

bool esp_at_wifi_init(void)
{
    return esp_at_write_command("AT+CWMODE=1", 2000);
}

bool esp_at_connect_wifi(const char *ssid, const char *pwd, const char *mac)
{
    if (ssid == NULL || pwd == NULL)
        return false;
    
    char cmd[128];
    int len = snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
    
    if (len < 0 || len >= sizeof(cmd)) {
        return false; 
    }
    
    if (mac) {
        int remain = sizeof(cmd) - len;
        if (remain > 0) {
            snprintf(cmd + len, remain, ",\"%s\"", mac);
        }
    }
    
    return esp_at_write_command(cmd, 20000);
}

static bool parse_cwjap_response(const char *response, esp_wifi_info_t *info)
{
    response = strstr(response, "+CWJAP:");
    if (response == NULL) return false;
    
    if (sscanf(response, "+CWJAP:\"%63[^\"]\",\"%17[^\"]\",%d,%d", 
               info->ssid, info->bssid, &info->channel, &info->rssi) != 4)
        return false;
    
    return true;
}

bool esp_at_get_wifi_info(esp_wifi_info_t *info)
{
    esp_lock();
    if (!esp_at_write_command("AT+CWJAP?", 2000))
    {
        esp_unlock();
        return false;
    }
    
    const char *resp = esp_at_get_response();
    
    if (parse_cwjap_response(resp, info))
    {
        info->connected = true;
        esp_unlock();
        return true;
    }
    
    esp_unlock();
    return false;
}

bool wifi_is_connected(void)
{
    esp_wifi_info_t info = {0};
    if (esp_at_get_wifi_info(&info))
    {
        return info.connected;
    }
    return false;
}

bool esp_at_sntp_init(void)
{
    esp_at_write_command("AT+CIPSNTPCFG=1,8,\"cn.pool.ntp.org\",\"ntp.aliyun.com\",\"ntp.tencent.com\"", 2000);
    return true;
}

static uint8_t month_str_to_num(const char *month_str)
{
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (uint8_t i = 0; i < 12; i++) {
        if (strncmp(month_str, months[i], 3) == 0) return i + 1;
    }
    return 0;
}

static uint8_t weekday_str_to_num(const char *weekday_str)
{
    const char *weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    for (uint8_t i = 0; i < 7; i++) {
        if (strncmp(weekday_str, weekdays[i], 3) == 0) return i + 1;
    }
    return 0;
}

static bool parse_cipsntptime_response(const char *response, esp_date_time_t *date)
{
    char weekday_str[8] = {0};
    char month_str[8] = {0};
    
    response = strstr(response, "+CIPSNTPTIME:");
    if (!response) return false;

    unsigned int temp_day, temp_hour, temp_minute, temp_second, temp_year;
    int parsed = sscanf(response, "+CIPSNTPTIME:%3s %3s %u %u:%u:%u %u", 
               weekday_str, month_str, 
               &temp_day, &temp_hour, &temp_minute, &temp_second, &temp_year);
    
    date->day = (uint8_t)temp_day;
    date->hour = (uint8_t)temp_hour;
    date->minute = (uint8_t)temp_minute;
    date->second = (uint8_t)temp_second;
    date->year = (uint16_t)temp_year;
    
    if (parsed != 7)
        return false;
    
    date->weekday = weekday_str_to_num(weekday_str);
    date->month = month_str_to_num(month_str);
    return true;
}

bool esp_at_sntp_get_time(esp_date_time_t *date)
{
    if (!esp_at_write_command("AT+CIPSNTPTIME?", 2000))
        return false;
    
    if (!parse_cipsntptime_response(esp_at_get_response(), date))
        return false;
    
    return true;
}

const char *esp_at_http_get(const char *url)
{
    if(strlen(url) > 512) {
        return NULL;
    }

    char cmd[600];
    int ret = snprintf(cmd, sizeof(cmd), "AT+HTTPCLIENT=2,1,\"%s\",,,2", url);
    if (ret < 0 || ret >= sizeof(cmd)) {
        return NULL;
    }
    
    bool ok = esp_at_write_command(cmd, 15000);
    
    if (!ok)
    {
        return NULL;
    }
    
    return esp_at_get_response();
}
