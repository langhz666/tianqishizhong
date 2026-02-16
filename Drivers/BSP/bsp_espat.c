#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "stm32f4xx_hal.h" // 请确保这里对应你的芯片型号，如 F1 则是 stm32f1xx_hal.h
#include "bsp_espat.h"

// 调试开关：1开启printf调试日志，0关闭
#define ESP_AT_DEBUG    0

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* * 引用外部定义的串口句柄。
 * 请确保在 main.c 中定义了 huart2 并完成了初始化。
 */
extern UART_HandleTypeDef huart2;

/* * 【关键修复】加上括号！
 * 修复了优先级问题：(&huart2)->Instance 才是合法的
 */
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

static char rxbuf[2048]; // 接收缓冲区

// 内部函数声明
static void esp_at_usart_write(const char *data);

/* * 底层接收函数（核心修复部分）
 * 增加了清除 ORE (Overrun Error) 的逻辑，防止高波特率下死锁
 */
static at_ack_t esp_at_usart_wait_receive(uint32_t timeout)
{
    uint32_t rxlen = 0;
    // 【修复】删除了未使用的 line 变量，消除警告
    uint32_t start = HAL_GetTick();
    
    // 清空缓冲区
    memset(rxbuf, 0, sizeof(rxbuf));
    
    while (rxlen < sizeof(rxbuf) - 1)
    {
        // 1. 关键修复：检查并清除 ORE 溢出错误标志
        // 如果数据来得太快，RXNE还没处理完，ORE就会置位，导致RXNE不再触发
        if(__HAL_UART_GET_FLAG(ESP_UART_HANDLE, UART_FLAG_ORE))
        {
            __HAL_UART_CLEAR_OREFLAG(ESP_UART_HANDLE);
            // 读取一次数据寄存器，抛弃错误数据
            (void)ESP_UART_HANDLE->Instance->DR; 
        }

        // 2. 检查是否有新数据 (RXNE)
        if (__HAL_UART_GET_FLAG(ESP_UART_HANDLE, UART_FLAG_RXNE) == SET)
        {
            // 读取数据
            uint8_t data = (uint8_t)(ESP_UART_HANDLE->Instance->DR & 0xFF);
            
            rxbuf[rxlen++] = data;
            
            // 每次遇到换行符，检查一次是否匹配关键词
            if (rxbuf[rxlen - 1] == '\n')
            {
                // 遍历关键词表
                for (uint32_t i = 0; i < ARRAY_SIZE(at_ack_matches); i++)
                {
                    // 使用 strstr 查找子串
                    if (strstr(rxbuf, at_ack_matches[i].string) != NULL)
                    {
                        return at_ack_matches[i].ack;
                    }
                }
            }
        }
        else
        {
            // 3. 超时检查
            if ((HAL_GetTick() - start) >= timeout)
            {
                return AT_ACK_NONE;
            }
        }
    }
    
    return AT_ACK_NONE;
}

/* 底层发送函数 */
static void esp_at_usart_write(const char *data)
{
    if (data && *data)
    {
        HAL_UART_Transmit(ESP_UART_HANDLE, (uint8_t*)data, strlen(data), 1000);
    }
    
    uint8_t newline[] = {'\r', '\n'};
    HAL_UART_Transmit(ESP_UART_HANDLE, newline, 2, 100);
}

/* 等待特定响应（主要是 ready） */
bool esp_at_wait_ready(uint32_t timeout)
{
    // 复用接收逻辑，等待 "ready"
    return esp_at_usart_wait_receive(timeout) == AT_ACK_READY;
}

/* 发送指令并等待 OK */
bool esp_at_write_command(const char *command, uint32_t timeout)
{
#if ESP_AT_DEBUG
    printf("[CMD] >> %s\n", command);
#endif

    esp_at_usart_write(command);
    at_ack_t ack = esp_at_usart_wait_receive(timeout);

#if ESP_AT_DEBUG
    if(ack == AT_ACK_OK) printf("[ACK] << OK\n");
    else if(ack == AT_ACK_ERROR) printf("[ACK] << ERROR\n");
    else printf("[ACK] << TIMEOUT/OTHER\n");
#endif

    return ack == AT_ACK_OK;
}

/* 获取最后一次响应的缓冲区内容 */
const char *esp_at_get_response(void)
{
    return rxbuf;
}

/* * 模块初始化函数（修复了初始化逻辑）
 */
bool esp_at_init(void)
{
    uint32_t start;
    uint32_t rxlen;

    // 清除可能存在的错误标志
    __HAL_UART_CLEAR_OREFLAG(ESP_UART_HANDLE);
    __HAL_UART_CLEAR_NEFLAG(ESP_UART_HANDLE);
    __HAL_UART_CLEAR_FEFLAG(ESP_UART_HANDLE);

    // 尝试不同的波特率
    const uint32_t baud_rates[] = {115200, 9600, 57600, 38400, 74880, 230400};
    
    for (int i = 0; i < 6; i++)
    {
        huart2.Init.BaudRate = baud_rates[i];
        if (HAL_UART_Init(&huart2) != HAL_OK)
        {
            continue;
        }
        
        memset(rxbuf, 0, sizeof(rxbuf));
        esp_at_usart_write("AT");
        
        start = HAL_GetTick();
        rxlen = 0;
        
        while ((HAL_GetTick() - start) < 1000 && rxlen < sizeof(rxbuf) - 1)
        {
            if (__HAL_UART_GET_FLAG(ESP_UART_HANDLE, UART_FLAG_RXNE) == SET)
            {
                uint8_t data = (uint8_t)(ESP_UART_HANDLE->Instance->DR & 0xFF);
                rxbuf[rxlen++] = data;
            }
        }
        
        if (strstr((char*)rxbuf, "OK") != NULL)
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
    while ((HAL_GetTick() - start) < 1000)
    {
        if (__HAL_UART_GET_FLAG(ESP_UART_HANDLE, UART_FLAG_RXNE) == SET)
        {
            (void)ESP_UART_HANDLE->Instance->DR;
        }
    }
    
    if (!esp_at_write_command("AT", 1000))
    {
        HAL_Delay(1000);
        if (!esp_at_write_command("AT", 1000))
        {
            return false;
        }
    }
    
    esp_at_write_command("ATE0", 500);
    
    return true;
}

bool esp_at_wifi_init(void)
{
    return esp_at_write_command("AT+CWMODE=1", 2000);
}

/* 连接 WiFi (增加了缓冲区安全检查) */
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

// --- 以下是解析相关辅助函数 ---

static bool parse_cwstate_response(const char *response, esp_wifi_info_t *info)
{
    response = strstr(response, "+CWSTATE:");
    if (response == NULL) return false;
    
    int wifi_state;
    // +CWSTATE:2,"SSID"
    if (sscanf(response, "+CWSTATE:%d,\"%63[^\"]", &wifi_state, info->ssid) != 2)
        return false;
    
    info->connected = (wifi_state == 2);
    return true;
}

static bool parse_cwjap_response(const char *response, esp_wifi_info_t *info)
{
    response = strstr(response, "+CWJAP:");
    if (response == NULL) return false;
    
    // +CWJAP:"SSID","MAC",...
    if (sscanf(response, "+CWJAP:\"%63[^\"]\",\"%17[^\"]\",%d,%d", 
               info->ssid, info->bssid, &info->channel, &info->rssi) != 4)
        return false;
    
    return true;
}

bool esp_at_get_wifi_info(esp_wifi_info_t *info)
{
    // 查询状态
    if (!esp_at_write_command("AT+CWSTATE?", 2000)) return false;
    if (!parse_cwstate_response(esp_at_get_response(), info)) return false;
    
    // 如果已连接，查询详细信息
    if (info->connected) {
        if (!esp_at_write_command("AT+CWJAP?", 2000)) return false;
        if (!parse_cwjap_response(esp_at_get_response(), info)) return false;
    }
    return true;
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

// --- SNTP 时间相关 ---

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

// --- HTTP 相关 ---

const char *esp_at_http_get(const char *url)
{
    if(strlen(url) > 512) {
        printf("[HTTP] URL too long\n");
        return NULL;
    }

    char cmd[600];
    int ret = snprintf(cmd, sizeof(cmd), "AT+HTTPCLIENT=2,1,\"%s\",,,2", url);
    if (ret < 0 || ret >= sizeof(cmd)) {
        printf("[HTTP] Command buffer overflow\n");
        return NULL;
    }

    printf("[HTTP] Sending: %s\n", cmd);
    
    bool ok = esp_at_write_command(cmd, 15000);
    
    if (!ok)
    {
        printf("[HTTP] Command failed\n");
        return NULL;
    }
    
    const char *response = esp_at_get_response();
    printf("[HTTP] Response length: %d\n", strlen(response));
    
    return response;
}
