/**
 * @file bsp_espat.c
 * @brief ESP8266/ESP32 ATָ������ģ��
 * 
 * ���ļ�ʵ����ͨ��UART��ESP8266/ESP32ģ��ͨ�ŵ�ATָ��������������
 * - ATָ�������Ӧ����
 * - WiFi���ӹ���
 * - SNTPʱ��ͬ��
 * - HTTP GET����
 * 
 * ͨ��Э�飺
 * - �����ʣ�115200 bps
 * - ����λ��8λ
 * - ֹͣλ��1λ
 * - У��λ����
 * 
 * ATָ����Ӧ��ʽ��
 * - �ɹ���������� + "\r\n" + "OK" + "\r\n"
 * - ʧ�ܣ�������� + "\r\n" + "ERROR" + "\r\n"
 * 
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "bsp_espat.h"

/*============================================================================*/
/*                             �궨��                                         */
/*============================================================================*/

/**
 * @brief ��������Ԫ�ظ���
 * @param arr ������
 * @return ����Ԫ�ظ���
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/*============================================================================*/
/*                             �ⲿ��������                                   */
/*============================================================================*/

extern UART_HandleTypeDef huart2;    /**< UART2�����������ESPģ��ͨ�� */
extern void esp_lock(void);          /**< ��ȡESPģ�黥���� */
extern void esp_unlock(void);        /**< �ͷ�ESPģ�黥���� */

/*============================================================================*/
/*                             ˽�к궨��                                     */
/*============================================================================*/

#define ESP_UART_HANDLE (&huart2)    /**< ESPģ��ʹ�õ�UART���ָ�� */

/*============================================================================*/
/*                             ˽�����Ͷ���                                   */
/*============================================================================*/

/**
 * @brief ATָ����Ӧ����ö��
 */
typedef enum
{
    AT_ACK_NONE,     /**< ����Ӧ/��ʱ */
    AT_ACK_OK,       /**< ��ӦOK */
    AT_ACK_ERROR,    /**< ��ӦERROR */
    AT_ACK_BUSY,     /**< ģ��æ */
    AT_ACK_READY,    /**< ģ����� */
} at_ack_t;

/**
 * @brief AT��Ӧƥ��ṹ��
 * ���ڽ���Ӧ�ַ���ӳ�䵽��Ӧ����
 */
typedef struct
{
    at_ack_t ack;         /**< ��Ӧ���� */
    const char *string;   /**< ��Ӧ�ַ��� */
} at_ack_match_t;

/*============================================================================*/
/*                             ˽�б���                                       */
/*============================================================================*/

/**
 * @brief AT��Ӧƥ���
 * �����˸���ATָ����Ӧ�ַ�������Ӧ���͵Ķ�Ӧ��ϵ
 */
static const at_ack_match_t at_ack_matches[] = 
{
    {AT_ACK_OK, "OK\r\n"},         /**< �ɹ���Ӧ */
    {AT_ACK_ERROR, "ERROR\r\n"},   /**< ������Ӧ */
    {AT_ACK_BUSY, "busy p..."},    /**< æµ��Ӧ */
    {AT_ACK_READY, "ready\r\n"},   /**< ������Ӧ */
};

static char rxbuf[2048];           /**< UART���ջ����� */

/*============================================================================*/
/*                             ˽�к�������                                   */
/*============================================================================*/

static void esp_at_usart_write(const char *data);

/*============================================================================*/
/*                             UARTͨ�ź���                                   */
/*============================================================================*/

/**
 * @brief �ȴ�������UART��Ӧ����
 * 
 * �˺���������ѯ��ʽ����UART���ݣ������յ�������Ӧ�������Ӧ���͡�
 * ��Ӧ�Ի��з�(\n)��βʱ����ƥ���顣
 * 
 * @param timeout ��ʱʱ�䣨���룩
 * @return at_ack_t ��Ӧ����ö��ֵ
 * 
 * @note �˺�����������ջ��������ٿ�ʼ����
 * @note ������ջ���������δ�յ�������Ӧ������AT_ACK_NONE
 */
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

/**
 * @brief ͨ��UART����ATָ��
 * 
 * ���͸�ʽ��ָ������ + "\r\n"
 * ATָ������Իس����н�β���ܱ�ESPģ��ʶ��
 * 
 * @param data Ҫ���͵�ָ���ַ���������\r\n��
 */
static void esp_at_usart_write(const char *data)
{
    if (data && *data)
    {
        HAL_UART_Transmit(ESP_UART_HANDLE, (uint8_t*)data, strlen(data), 1000);
    }
    
    uint8_t newline[] = {'\r', '\n'};
    HAL_UART_Transmit(ESP_UART_HANDLE, newline, 2, 100);
}

/*============================================================================*/
/*                             ATָ���������                                 */
/*============================================================================*/

/**
 * @brief �ȴ�ESPģ�����
 * 
 * �ȴ�ESPģ���ϵ���ɺ���"ready"��Ӧ
 * 
 * @param timeout ��ʱʱ�䣨���룩
 * @return true ģ�����
 * @return false ��ʱδ�յ�������Ӧ
 */
bool esp_at_wait_ready(uint32_t timeout)
{
    return esp_at_usart_wait_receive(timeout) == AT_ACK_READY;
}

/**
 * @brief ����ATָ��ȴ���Ӧ������������
 * 
 * @param command ATָ���ַ���������\r\n��
 * @param timeout ��ʱʱ�䣨���룩
 * @return true ָ��ִ�гɹ����յ�OK��
 * @return false ָ��ִ��ʧ�ܻ�ʱ
 * 
 * @note �˺��������̰߳�ȫ�ģ������񻷾�����ʹ��esp_at_write_command_locked
 */
bool esp_at_write_command(const char *command, uint32_t timeout)
{
    esp_at_usart_write(command);
    at_ack_t ack = esp_at_usart_wait_receive(timeout);
    return ack == AT_ACK_OK;
}

/**
 * @brief ����ATָ��ȴ���Ӧ����������������
 * 
 * �ڶ����񻷾��£�ʹ�ô˺������Է�ֹ�������ͬʱ����ESPģ�鵼�µĳ�ͻ
 * 
 * @param command ATָ���ַ���������\r\n��
 * @param timeout ��ʱʱ�䣨���룩
 * @return true ָ��ִ�гɹ�
 * @return false ָ��ִ��ʧ�ܻ�ʱ
 */
bool esp_at_write_command_locked(const char *command, uint32_t timeout)
{
    esp_lock();
    bool result = esp_at_write_command(command, timeout);
    esp_unlock();
    return result;
}

/**
 * @brief ��ȡ���һ��ATָ�����Ӧ����
 * 
 * @return const char* ��Ӧ�ַ���ָ�루ָ���ڲ���������
 * 
 * @note ���ص�ָ��ָ���ڲ����������´ν��ջḲ������
 */
const char *esp_at_get_response(void)
{
    return rxbuf;
}

/*============================================================================*/
/*                             ESPģ���ʼ������                              */
/*============================================================================*/

/**
 * @brief ��ʼ��ESP ATģ��
 * 
 * ��ʼ�����̣�
 * 1. ���UART�����־
 * 2. �ȴ�ģ���ϵ��ȶ���5�룩
 * 3. �Զ���Ⲩ���ʣ���ǰ�̶�115200��
 * 4. ����ATָ�����ͨ��
 * 5. ��λģ��
 * 6. �ȴ�ģ���������
 * 7. �رջ��ԣ�ATE0��
 * 
 * @return true ��ʼ���ɹ�
 * @return false ��ʼ��ʧ��
 * 
 * @note ��ʼ������Լ��15��
 */
bool esp_at_init(void)
{
    uint32_t start;
    uint32_t rxlen;

    __HAL_UART_CLEAR_OREFLAG(ESP_UART_HANDLE);
    __HAL_UART_CLEAR_NEFLAG(ESP_UART_HANDLE);
    __HAL_UART_CLEAR_FEFLAG(ESP_UART_HANDLE);

    HAL_Delay(5000);

    const uint32_t baud_rates[] = {115200, 57600, 38400, 9600, 74880, 230400};

    for (uint32_t i = 0; i < ARRAY_SIZE(baud_rates); i++)
    {
        printf("[AT] probing baud: %lu\n", baud_rates[i]);
        huart2.Init.BaudRate = baud_rates[i];
        if (HAL_UART_Init(&huart2) != HAL_OK)
        {
            printf("[AT] uart init failed at %lu\n", baud_rates[i]);
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
        
        for (uint8_t retry = 0; retry < 3; retry++)
        {
            esp_at_usart_write("AT");

            start = HAL_GetTick();
            rxlen = 0;

            while ((HAL_GetTick() - start) < 1200 && rxlen < sizeof(rxbuf) - 1)
            {
                if (__HAL_UART_GET_FLAG(ESP_UART_HANDLE, UART_FLAG_RXNE) == SET)
                {
                    uint8_t data = (uint8_t)(ESP_UART_HANDLE->Instance->DR & 0xFF);
                    if (rxlen < sizeof(rxbuf) - 1) {
                        rxbuf[rxlen++] = data;
                    }
                }
            }

            rxbuf[rxlen] = '\0';
            if (strstr((char*)rxbuf, "OK") != NULL || strstr((char*)rxbuf, "ERROR") != NULL || strstr((char*)rxbuf, "ready") != NULL)
            {
                printf("[AT] baud locked: %lu\n", baud_rates[i]);
                goto baud_found;
            }
        }

        printf("[AT] no valid response at %lu\n", baud_rates[i]);
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

/*============================================================================*/
/*                             WiFi���ܺ���                                   */
/*============================================================================*/

/**
 * @brief ��ʼ��WiFiЭ��ջ
 * 
 * ����WiFi����ģʽΪStationģʽ���ͻ���ģʽ��
 * 
 * @return true ���óɹ�
 * @return false ����ʧ��
 * 
 * @note AT+CWMODE=1 ��ʾStationģʽ
 */
bool esp_at_wifi_init(void)
{
    return esp_at_write_command("AT+CWMODE=1", 2000);
}

/**
 * @brief ���ӵ�WiFi�ȵ�
 * 
 * @param ssid WiFi��������
 * @param pwd WiFi����
 * @param mac Ŀ��AP��MAC��ַ����ѡ������ָ�������ض�AP��
 * @return true ���ӳɹ�
 * @return false ����ʧ��
 * 
 * @note ���ӳ�ʱʱ��Ϊ20��
 * @note ���ssid��pwdΪNULL��ֱ�ӷ���ʧ��
 */
bool esp_at_connect_wifi(const char *ssid, const char *pwd, const char *mac)
{
    if (ssid == NULL || pwd == NULL)
        return false;

    /* Some firmwares return non-OK when already connected, so pre-check first. */
    esp_wifi_info_t current = {0};
    if (esp_at_get_wifi_info(&current) && current.connected)
    {
        if (strcmp(current.ssid, ssid) == 0)
        {
            return true;
        }
    }
    
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
    
    if (esp_at_write_command(cmd, 30000))
    {
        return true;
    }

    /* Fallback: connection may still be up even if CWJAP did not return OK. */
    memset(&current, 0, sizeof(current));
    if (esp_at_get_wifi_info(&current) && current.connected)
    {
        if (strcmp(current.ssid, ssid) == 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief ����WiFi������Ϣ��Ӧ
 * 
 * ����AT+CWJAP?ָ�����Ӧ����ʽ���£�
 * +CWJAP:"SSID","BSSID",Channel,RSSI
 * 
 * @param response ��Ӧ�ַ���
 * @param info �����WiFi��Ϣ�ṹ��
 * @return true �����ɹ�
 * @return false ����ʧ��
 */
static bool parse_cwjap_response(const char *response, esp_wifi_info_t *info)
{
    response = strstr(response, "+CWJAP:");
    if (response == NULL) return false;
    
    if (sscanf(response, "+CWJAP:\"%63[^\"]\",\"%17[^\"]\",%d,%d", 
               info->ssid, info->bssid, &info->channel, &info->rssi) != 4)
        return false;
    
    return true;
}

/**
 * @brief ��ȡ��ǰWiFi������Ϣ
 * 
 * @param info �����WiFi��Ϣ�ṹ��ָ��
 * @return true ��ȡ�ɹ�
 * @return false δ���ӻ��ȡʧ��
 * 
 * @note �˺������̰߳�ȫ�ģ��ڲ�ʹ���˻�����
 */
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

/**
 * @brief ���WiFi�Ƿ�������
 * 
 * @return true ������
 * @return false δ����
 */
bool wifi_is_connected(void)
{
    esp_wifi_info_t info = {0};
    if (esp_at_get_wifi_info(&info))
    {
        return info.connected;
    }
    return false;
}

/*============================================================================*/
/*                             SNTPʱ��ͬ������                               */
/*============================================================================*/

/**
 * @brief ��ʼ��SNTPʱ��ͬ������
 * 
 * ����SNTP��������
 * - cn.pool.ntp.org���й�NTP�أ�
 * - ntp.aliyun.com��������NTP��
 * - ntp.tencent.com����Ѷ��NTP��
 * 
 * ʱ������Ϊ������������ʱ�䣬UTC+8��
 * 
 * @return true ���óɹ�
 * @return false ����ʧ��
 */
bool esp_at_sntp_init(void)
{
    esp_at_write_command("AT+CIPSNTPCFG=1,8,\"cn.pool.ntp.org\",\"ntp.aliyun.com\",\"ntp.tencent.com\"", 2000);
    return true;
}

/**
 * @brief ���·��ַ���ת��Ϊ����
 * 
 * @param month_str �·��ַ�������"Jan", "Feb"�ȣ�
 * @return uint8_t �·����֣�1-12������Ч���뷵��0
 */
static uint8_t month_str_to_num(const char *month_str)
{
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (uint8_t i = 0; i < 12; i++) {
        if (strncmp(month_str, months[i], 3) == 0) return i + 1;
    }
    return 0;
}

/**
 * @brief �������ַ���ת��Ϊ����
 * 
 * @param weekday_str �����ַ�������"Mon", "Tue"�ȣ�
 * @return uint8_t �������֣�1-7��1=��һ������Ч���뷵��0
 */
static uint8_t weekday_str_to_num(const char *weekday_str)
{
    const char *weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    for (uint8_t i = 0; i < 7; i++) {
        if (strncmp(weekday_str, weekdays[i], 3) == 0) return i + 1;
    }
    return 0;
}

/**
 * @brief ����SNTPʱ����Ӧ
 * 
 * ����AT+CIPSNTPTIME?ָ�����Ӧ����ʽ���£�
 * +CIPSNTPTIME:Thu Jan 01 08:00:00 1970
 * 
 * @param response ��Ӧ�ַ���
 * @param date ���������ʱ��ṹ��
 * @return true �����ɹ�
 * @return false ����ʧ��
 */
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

/**
 * @brief ��SNTP��������ȡ��ǰʱ��
 * 
 * @param date ���������ʱ��ṹ��ָ��
 * @return true ��ȡ�ɹ�
 * @return false ��ȡʧ��
 * 
 * @note ��Ҫ�ȵ���esp_at_sntp_init()��ʼ��SNTP����
 * @note ��ҪWiFi������
 */
bool esp_at_sntp_get_time(esp_date_time_t *date)
{
    if (!esp_at_write_command("AT+CIPSNTPTIME?", 2000))
        return false;
    
    if (!parse_cipsntptime_response(esp_at_get_response(), date))
        return false;
    
    return true;
}

/*============================================================================*/
/*                             HTTP�ͻ��˺���                                 */
/*============================================================================*/

/**
 * @brief ����HTTP GET����
 * 
 * @param url �����URL��ַ
 * @return const char* ��Ӧ����ָ�룬ʧ�ܷ���NULL
 * 
 * @note URL���Ȳ��ܳ���512�ֽ�
 * @note ����ʱʱ��Ϊ15��
 * @note ���ص�ָ��ָ���ڲ����������´�����Ḳ������
 */
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
