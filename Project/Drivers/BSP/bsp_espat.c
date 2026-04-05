/**
 * @file bsp_espat.c
 * @brief ESP8266/ESP32 AT指令驱动模块
 * 
 * 本文件实现了通过UART与ESP8266/ESP32模块通信的AT指令驱动，包括：
 * - AT指令发送与响应解析
 * - WiFi连接管理
 * - SNTP时间同步
 * - HTTP GET请求
 * 
 * 通信协议：
 * - 波特率：115200 bps
 * - 数据位：8位
 * - 停止位：1位
 * - 校验位：无
 * 
 * AT指令响应格式：
 * - 成功：命令回显 + "\r\n" + "OK" + "\r\n"
 * - 失败：命令回显 + "\r\n" + "ERROR" + "\r\n"
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
/*                             宏定义                                         */
/*============================================================================*/

/**
 * @brief 计算数组元素个数
 * @param arr 数组名
 * @return 数组元素个数
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/*============================================================================*/
/*                             外部变量声明                                   */
/*============================================================================*/

extern UART_HandleTypeDef huart2;    /**< UART2句柄，用于与ESP模块通信 */
extern void esp_lock(void);          /**< 获取ESP模块互斥锁 */
extern void esp_unlock(void);        /**< 释放ESP模块互斥锁 */

/*============================================================================*/
/*                             私有宏定义                                     */
/*============================================================================*/

#define ESP_UART_HANDLE (&huart2)    /**< ESP模块使用的UART句柄指针 */

/*============================================================================*/
/*                             私有类型定义                                   */
/*============================================================================*/

/**
 * @brief AT指令响应类型枚举
 */
typedef enum
{
    AT_ACK_NONE,     /**< 无响应/超时 */
    AT_ACK_OK,       /**< 响应OK */
    AT_ACK_ERROR,    /**< 响应ERROR */
    AT_ACK_BUSY,     /**< 模块忙 */
    AT_ACK_READY,    /**< 模块就绪 */
} at_ack_t;

/**
 * @brief AT响应匹配结构体
 * 用于将响应字符串映射到响应类型
 */
typedef struct
{
    at_ack_t ack;         /**< 响应类型 */
    const char *string;   /**< 响应字符串 */
} at_ack_match_t;

/*============================================================================*/
/*                             私有变量                                       */
/*============================================================================*/

/**
 * @brief AT响应匹配表
 * 定义了各种AT指令响应字符串与响应类型的对应关系
 */
static const at_ack_match_t at_ack_matches[] = 
{
    {AT_ACK_OK, "OK\r\n"},         /**< 成功响应 */
    {AT_ACK_ERROR, "ERROR\r\n"},   /**< 错误响应 */
    {AT_ACK_BUSY, "busy p..."},    /**< 忙碌响应 */
    {AT_ACK_READY, "ready\r\n"},   /**< 就绪响应 */
};

static char rxbuf[2048];           /**< UART接收缓冲区 */

/*============================================================================*/
/*                             私有函数声明                                   */
/*============================================================================*/

static void esp_at_usart_write(const char *data);

/*============================================================================*/
/*                             UART通信函数                                   */
/*============================================================================*/

/**
 * @brief 等待并接收UART响应数据
 * 
 * 此函数采用轮询方式接收UART数据，并在收到完整响应后解析响应类型。
 * 响应以换行符(\n)结尾时进行匹配检查。
 * 
 * @param timeout 超时时间（毫秒）
 * @return at_ack_t 响应类型枚举值
 * 
 * @note 此函数会清除接收缓冲区后再开始接收
 * @note 如果接收缓冲区满仍未收到完整响应，返回AT_ACK_NONE
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
 * @brief 通过UART发送AT指令
 * 
 * 发送格式：指令内容 + "\r\n"
 * AT指令必须以回车换行结尾才能被ESP模块识别
 * 
 * @param data 要发送的指令字符串（不含\r\n）
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
/*                             AT指令基础函数                                 */
/*============================================================================*/

/**
 * @brief 等待ESP模块就绪
 * 
 * 等待ESP模块上电完成后发送"ready"响应
 * 
 * @param timeout 超时时间（毫秒）
 * @return true 模块就绪
 * @return false 超时未收到就绪响应
 */
bool esp_at_wait_ready(uint32_t timeout)
{
    return esp_at_usart_wait_receive(timeout) == AT_ACK_READY;
}

/**
 * @brief 发送AT指令并等待响应（无锁保护）
 * 
 * @param command AT指令字符串（不含\r\n）
 * @param timeout 超时时间（毫秒）
 * @return true 指令执行成功（收到OK）
 * @return false 指令执行失败或超时
 * 
 * @note 此函数不是线程安全的，多任务环境下请使用esp_at_write_command_locked
 */
bool esp_at_write_command(const char *command, uint32_t timeout)
{
    esp_at_usart_write(command);
    at_ack_t ack = esp_at_usart_wait_receive(timeout);
    return ack == AT_ACK_OK;
}

/**
 * @brief 发送AT指令并等待响应（带互斥锁保护）
 * 
 * 在多任务环境下，使用此函数可以防止多个任务同时访问ESP模块导致的冲突
 * 
 * @param command AT指令字符串（不含\r\n）
 * @param timeout 超时时间（毫秒）
 * @return true 指令执行成功
 * @return false 指令执行失败或超时
 */
bool esp_at_write_command_locked(const char *command, uint32_t timeout)
{
    esp_lock();
    bool result = esp_at_write_command(command, timeout);
    esp_unlock();
    return result;
}

/**
 * @brief 获取最近一次AT指令的响应数据
 * 
 * @return const char* 响应字符串指针（指向内部缓冲区）
 * 
 * @note 返回的指针指向内部缓冲区，下次接收会覆盖内容
 */
const char *esp_at_get_response(void)
{
    return rxbuf;
}

/*============================================================================*/
/*                             ESP模块初始化函数                              */
/*============================================================================*/

/**
 * @brief 初始化ESP AT模块
 * 
 * 初始化流程：
 * 1. 清除UART错误标志
 * 2. 等待模块上电稳定（5秒）
 * 3. 自动检测波特率（当前固定115200）
 * 4. 发送AT指令测试通信
 * 5. 复位模块
 * 6. 等待模块重启完成
 * 7. 关闭回显（ATE0）
 * 
 * @return true 初始化成功
 * @return false 初始化失败
 * 
 * @note 初始化过程约需15秒
 */
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

/*============================================================================*/
/*                             WiFi功能函数                                   */
/*============================================================================*/

/**
 * @brief 初始化WiFi协议栈
 * 
 * 设置WiFi工作模式为Station模式（客户端模式）
 * 
 * @return true 设置成功
 * @return false 设置失败
 * 
 * @note AT+CWMODE=1 表示Station模式
 */
bool esp_at_wifi_init(void)
{
    return esp_at_write_command("AT+CWMODE=1", 2000);
}

/**
 * @brief 连接到WiFi热点
 * 
 * @param ssid WiFi网络名称
 * @param pwd WiFi密码
 * @param mac 目标AP的MAC地址（可选，用于指定连接特定AP）
 * @return true 连接成功
 * @return false 连接失败
 * 
 * @note 连接超时时间为20秒
 * @note 如果ssid或pwd为NULL，直接返回失败
 */
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

/**
 * @brief 解析WiFi连接信息响应
 * 
 * 解析AT+CWJAP?指令的响应，格式如下：
 * +CWJAP:"SSID","BSSID",Channel,RSSI
 * 
 * @param response 响应字符串
 * @param info 输出的WiFi信息结构体
 * @return true 解析成功
 * @return false 解析失败
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
 * @brief 获取当前WiFi连接信息
 * 
 * @param info 输出的WiFi信息结构体指针
 * @return true 获取成功
 * @return false 未连接或获取失败
 * 
 * @note 此函数是线程安全的，内部使用了互斥锁
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
 * @brief 检查WiFi是否已连接
 * 
 * @return true 已连接
 * @return false 未连接
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
/*                             SNTP时间同步函数                               */
/*============================================================================*/

/**
 * @brief 初始化SNTP时间同步服务
 * 
 * 配置SNTP服务器：
 * - cn.pool.ntp.org（中国NTP池）
 * - ntp.aliyun.com（阿里云NTP）
 * - ntp.tencent.com（腾讯云NTP）
 * 
 * 时区设置为东八区（北京时间，UTC+8）
 * 
 * @return true 配置成功
 * @return false 配置失败
 */
bool esp_at_sntp_init(void)
{
    esp_at_write_command("AT+CIPSNTPCFG=1,8,\"cn.pool.ntp.org\",\"ntp.aliyun.com\",\"ntp.tencent.com\"", 2000);
    return true;
}

/**
 * @brief 将月份字符串转换为数字
 * 
 * @param month_str 月份字符串（如"Jan", "Feb"等）
 * @return uint8_t 月份数字（1-12），无效输入返回0
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
 * @brief 将星期字符串转换为数字
 * 
 * @param weekday_str 星期字符串（如"Mon", "Tue"等）
 * @return uint8_t 星期数字（1-7，1=周一），无效输入返回0
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
 * @brief 解析SNTP时间响应
 * 
 * 解析AT+CIPSNTPTIME?指令的响应，格式如下：
 * +CIPSNTPTIME:Thu Jan 01 08:00:00 1970
 * 
 * @param response 响应字符串
 * @param date 输出的日期时间结构体
 * @return true 解析成功
 * @return false 解析失败
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
 * @brief 从SNTP服务器获取当前时间
 * 
 * @param date 输出的日期时间结构体指针
 * @return true 获取成功
 * @return false 获取失败
 * 
 * @note 需要先调用esp_at_sntp_init()初始化SNTP服务
 * @note 需要WiFi已连接
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
/*                             HTTP客户端函数                                 */
/*============================================================================*/

/**
 * @brief 发送HTTP GET请求
 * 
 * @param url 请求的URL地址
 * @return const char* 响应内容指针，失败返回NULL
 * 
 * @note URL长度不能超过512字节
 * @note 请求超时时间为15秒
 * @note 返回的指针指向内部缓冲区，下次请求会覆盖内容
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
