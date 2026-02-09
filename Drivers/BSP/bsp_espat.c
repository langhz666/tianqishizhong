#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "stm32f4xx_hal.h" // 根据你的具体芯片型号，可能是 stm32f1xx_hal.h 等
#include "bsp_espat.h"

// 调试开关：1开启printf调试，0关闭
#define ESP_AT_DEBUG    0

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* * 引用外部定义的串口句柄。
 * 请确保在 main.c 或 usart.c 中定义了 huart2 并完成了初始化。
 */
extern UART_HandleTypeDef huart2;
#define ESP_UART_HANDLE &huart2

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

static char rxbuf[1024];

static void esp_at_usart_write(const char *data);

/* * 串口初始化函数
 * 在 HAL 库中，硬件初始化通常由 CubeMX 生成的代码 (main.c) 完成。
 * 这里仅做占位或放置特定的重置逻辑。
 */
static void esp_at_usart_init(void)
{
    // 如果需要清除之前的错误状态，可以在这里添加
    // 例如：__HAL_UART_CLEAR_OREFLAG(ESP_UART_HANDLE);
}

bool esp_at_init(void)
{
    esp_at_usart_init();
    
    // 给模块上电后一点缓冲时间
    HAL_Delay(500);
    
    // 发送 AT 测试指令，多发几次确保同步
    esp_at_write_command("AT", 100);
    
    if (!esp_at_write_command("AT", 100))
        return false;
        
    if (!esp_at_write_command("AT+RESTORE", 2000))
        return false;
        
    if (!esp_at_wait_ready(5000))
        return false;
    
    return true;
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

static at_ack_t match_internal_ack(const char *str)
{
    for (uint32_t i = 0; i < ARRAY_SIZE(at_ack_matches); i++)
    {
        // 使用 strstr 进行子串匹配，提高对乱码或前缀字符的容错性
        if (strstr(str, at_ack_matches[i].string) != NULL)
            return at_ack_matches[i].ack;
    }
    
    return AT_ACK_NONE;
}

static at_ack_t esp_at_usart_wait_receive(uint32_t timeout)
{
    uint32_t rxlen = 0;
    const char *line = rxbuf;
    uint32_t start = HAL_GetTick(); // 使用 HAL 库的毫秒计数器
    
    rxbuf[0] = '\0';
    
    while (rxlen < sizeof(rxbuf) - 1)
    {
        // 检查 RXNE (接收数据寄存器非空) 标志位
        if (__HAL_UART_GET_FLAG(ESP_UART_HANDLE, UART_FLAG_RXNE) == SET)
        {
            // 读取数据寄存器 DR
            uint8_t data = (uint8_t)(ESP_UART_HANDLE->Instance->DR & 0xFF);
            
            rxbuf[rxlen++] = data;
            rxbuf[rxlen] = '\0';
            
            if (rxbuf[rxlen - 1] == '\n')
            {
                at_ack_t ack = match_internal_ack(line);
                if (ack != AT_ACK_NONE)
                    return ack;
                line = rxbuf + rxlen;
            }
        }
        else
        {
            // 超时检查
            if ((HAL_GetTick() - start) >= timeout)
            {
                return AT_ACK_NONE;
            }
        }
    }
    
    return AT_ACK_NONE;
}

bool esp_at_wait_ready(uint32_t timeout)
{
    return esp_at_usart_wait_receive(timeout) == AT_ACK_READY;
}

bool esp_at_write_command(const char *command, uint32_t timeout)
{
#if ESP_AT_DEBUG
    printf("[DEBUG] Send: %s\n", command);
#endif

    esp_at_usart_write(command);
    at_ack_t ack = esp_at_usart_wait_receive(timeout);

#if ESP_AT_DEBUG
    printf("[DEBUG] Response:\n%s\n", rxbuf);
#endif

    return ack == AT_ACK_OK;
}

const char *esp_at_get_response(void)
{
    return rxbuf;
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
    // AT+CWJAP="SSID","PWD"
    int len = snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
    
    if (mac)
        snprintf(cmd + len, sizeof(cmd) - len, ",\"%s\"", mac);
    
    // 连接 WiFi 可能会比较慢，给予 20秒 超时
    return esp_at_write_command(cmd, 20000);
}

static bool parse_cwstate_response(const char *response, esp_wifi_info_t *info)
{
    // 响应示例: +CWSTATE:2,"Xiaomi Mi MIX 3_5577"
    response = strstr(response, "+CWSTATE:");
    if (response == NULL)
        return false;
    
    int wifi_state;
    // 解析状态和 SSID
    if (sscanf(response, "+CWSTATE:%d,\"%63[^\"]", &wifi_state, info->ssid) != 2)
        return false;
    
    info->connected = (wifi_state == 2);
    
    return true;
}

static bool parse_cwjap_response(const char *response, esp_wifi_info_t *info)
{
    // 响应示例: +CWJAP:"Xiaomi Mi MIX 3_5577","da:b5:3a:e3:2f:60",9,-48,0,1,3,0,1
    response = strstr(response, "+CWJAP:");
    if (response == NULL)
        return false;
    
    // 解析 SSID, BSSID(MAC), Channel, RSSI
    // 您的头文件中 bssid 长度定义为 18，这里用 %17[^\"] 限制读取长度防止溢出
    if (sscanf(response, "+CWJAP:\"%63[^\"]\",\"%17[^\"]\",%d,%d", 
               info->ssid, info->bssid, &info->channel, &info->rssi) != 4)
        return false;
    
    return true;
}

bool esp_at_get_wifi_info(esp_wifi_info_t *info)
{
    if (!esp_at_write_command("AT+CWSTATE?", 2000))
        return false;
    
    if (!parse_cwstate_response(esp_at_get_response(), info))
        return false;
    
    if (!esp_at_write_command("AT+CWJAP?", 2000))
        return false;
    
    if (!parse_cwjap_response(esp_at_get_response(), info))
        return false;
    
    return true;
}

bool wifi_is_connected(void)
{
    esp_wifi_info_t info;
    if (esp_at_get_wifi_info(&info))
    {
        return info.connected;
    }
    return false;
}

bool esp_at_sntp_init(void)
{
    // 设置 SNTP 服务器，时区 +8
    if (!esp_at_write_command("AT+CIPSNTPCFG=1,8", 2000))
        return false;
    
    return true;
}

static uint8_t month_str_to_num(const char *month_str)
{
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (uint8_t i = 0; i < 12; i++)
    {
        if (strncmp(month_str, months[i], 3) == 0)
        {
            return i + 1;
        }
    }
    return 0;
}

static uint8_t weekday_str_to_num(const char *weekday_str)
{
    const char *weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    for (uint8_t i = 0; i < 7; i++) {
        if (strncmp(weekday_str, weekdays[i], 3) == 0)
        {
            return i + 1;
        }
    }
    return 0;
}

static bool parse_cipsntptime_response(const char *response, esp_date_time_t *date)
{
    // 响应示例: +CIPSNTPTIME:Sun Jul 27 14:07:19 2025
    char weekday_str[8];
    char month_str[8];
    response = strstr(response, "+CIPSNTPTIME:");
    if (!response) return false;

    if (sscanf(response, "+CIPSNTPTIME:%3s %3s %hhu %hhu:%hhu:%hhu %hu", 
               weekday_str, month_str, 
               &date->day, &date->hour, &date->minute, &date->second, &date->year) != 7)
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
    char *txbuf = rxbuf; // 复用接收缓冲区作为发送缓冲区
    
    // 注意：检查 URL 长度防止缓冲区溢出
    int ret = snprintf(txbuf, sizeof(rxbuf), "AT+HTTPCLIENT=2,1,\"%s\",,,2", url);
    if (ret < 0 || ret >= sizeof(rxbuf)) return NULL;

    // HTTP 请求可能受网络影响，设置 10秒 超时
    bool ok = esp_at_write_command(txbuf, 10000);
    return ok ? esp_at_get_response() : NULL;
}
