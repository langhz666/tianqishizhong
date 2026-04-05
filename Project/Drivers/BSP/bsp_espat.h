/**
 * @file bsp_espat.h
 * @brief ESP8266/ESP32 AT指令驱动模块头文件
 * 
 * 本头文件定义了ESP AT模块驱动的数据结构和函数接口。
 * 支持ESP8266和ESP32系列模块，通过UART进行AT指令通信。
 * 
 * 主要功能：
 * - AT指令基础通信
 * - WiFi连接管理
 * - SNTP网络时间同步
 * - HTTP客户端请求
 * 
 * 使用示例：
 * @code
 * // 初始化ESP模块
 * esp_at_init();
 * 
 * // 初始化WiFi并连接
 * esp_at_wifi_init();
 * esp_at_connect_wifi("SSID", "password", NULL);
 * 
 * // 获取网络时间
 * esp_at_sntp_init();
 * esp_date_time_t time;
 * esp_at_sntp_get_time(&time);
 * @endcode
 * 
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __ESP_AT_H__
#define __ESP_AT_H__

#include <stdbool.h>
#include <stdint.h>

/*============================================================================*/
/*                             数据结构定义                                   */
/*============================================================================*/

/**
 * @brief WiFi连接信息结构体
 * 
 * 存储当前WiFi连接的详细信息，包括网络名称、信号强度等
 */
typedef struct
{
	char ssid[64];      /**< WiFi网络名称（SSID），最大63字符 */
	char bssid[18];     /**< AP的MAC地址，格式如"aa:bb:cc:dd:ee:ff" */
	int channel;        /**< WiFi信道号，范围1-13 */
	int rssi;           /**< 信号强度（dBm），负值，越接近0信号越强 */
	bool connected;     /**< 连接状态标志，true表示已连接 */
} esp_wifi_info_t;

/**
 * @brief 日期时间结构体
 * 
 * 存储从SNTP服务器获取的网络时间信息
 * 所有字段均为无符号整数，便于直接使用
 */
typedef struct
{
    uint16_t year;      /**< 公历年份，如2026 */
    uint8_t month;      /**< 月份，范围1-12 */
    uint8_t day;        /**< 日期，范围1-31 */
    uint8_t hour;       /**< 小时，范围0-23（24小时制） */
    uint8_t minute;     /**< 分钟，范围0-59 */
    uint8_t second;     /**< 秒，范围0-59 */
    uint8_t weekday;    /**< 星期，范围1-7（1=周一，7=周日） */
} esp_date_time_t;

/*============================================================================*/
/*                             AT指令基础函数                                 */
/*============================================================================*/

/**
 * @brief 初始化ESP AT模块
 * 
 * 执行模块初始化流程，包括波特率检测、模块复位、关闭回显等
 * 
 * @return true 初始化成功
 * @return false 初始化失败（模块无响应或通信错误）
 * 
 * @note 初始化过程约需15秒，请确保模块已正确上电
 */
bool esp_at_init(void);

/**
 * @brief 等待ESP模块就绪
 * 
 * 阻塞等待ESP模块发送"ready"响应，用于模块复位后的等待
 * 
 * @param timeout 超时时间（毫秒）
 * @return true 模块已就绪
 * @return false 等待超时
 */
bool esp_at_wait_ready(uint32_t timeout);

/**
 * @brief 发送AT指令并等待响应
 * 
 * 发送AT指令并阻塞等待响应，函数会自动添加\r\n结尾
 * 
 * @param command AT指令字符串（不含\r\n）
 * @param timeout 超时时间（毫秒）
 * @return true 指令执行成功（收到OK响应）
 * @return false 指令执行失败或超时
 * 
 * @warning 此函数不是线程安全的，多任务环境请使用esp_at_write_command_locked
 */
bool esp_at_write_command(const char *command, uint32_t timeout);

/**
 * @brief 获取最近一次AT指令的响应数据
 * 
 * 返回内部接收缓冲区的指针，包含ESP模块的完整响应
 * 
 * @return const char* 响应字符串指针
 * 
 * @note 返回的指针指向内部静态缓冲区，下次接收会覆盖内容
 * @note 如果需要保存响应内容，请自行复制
 */
const char *esp_at_get_response(void);

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
 */
bool esp_at_wifi_init(void);

/**
 * @brief 连接到WiFi热点
 * 
 * 使用指定的SSID和密码连接WiFi网络
 * 
 * @param ssid WiFi网络名称
 * @param pwd WiFi密码
 * @param mac 目标AP的MAC地址（可选，传NULL即可）
 * @return true 连接成功
 * @return false 连接失败
 * 
 * @note 连接超时时间为20秒
 */
bool esp_at_connect_wifi(const char *ssid, const char *pwd, const char *mac);

/**
 * @brief 获取当前WiFi连接信息
 * 
 * 查询当前已连接的WiFi网络详细信息
 * 
 * @param info 输出的WiFi信息结构体指针
 * @return true 获取成功
 * @return false 未连接或获取失败
 */
bool esp_at_get_wifi_info(esp_wifi_info_t *info);

/**
 * @brief 检查WiFi是否已连接
 * 
 * 快速检查WiFi连接状态
 * 
 * @return true 已连接到WiFi网络
 * @return false 未连接
 */
bool wifi_is_connected(void);

/*============================================================================*/
/*                             SNTP时间同步函数                               */
/*============================================================================*/

/**
 * @brief 初始化SNTP时间同步服务
 * 
 * 配置SNTP服务器和时区，使用中国NTP服务器池
 * 时区设置为东八区（北京时间，UTC+8）
 * 
 * @return true 配置成功
 * @return false 配置失败
 * 
 * @note 需要先连接WiFi才能正常使用SNTP服务
 */
bool esp_at_sntp_init(void);

/**
 * @brief 从SNTP服务器获取当前时间
 * 
 * 从配置的NTP服务器获取准确的网络时间
 * 
 * @param date 输出的日期时间结构体指针
 * @return true 获取成功
 * @return false 获取失败
 * 
 * @note 需要先调用esp_at_sntp_init()初始化SNTP服务
 * @note 需要WiFi已连接
 */
bool esp_at_sntp_get_time(esp_date_time_t *date);

/*============================================================================*/
/*                             HTTP客户端函数                                 */
/*============================================================================*/

/**
 * @brief 发送HTTP GET请求
 * 
 * 向指定URL发送HTTP GET请求并获取响应内容
 * 
 * @param url 请求的完整URL地址
 * @return const char* 响应内容指针，失败返回NULL
 * 
 * @note URL长度不能超过512字节
 * @note 请求超时时间为15秒
 * @note 返回的指针指向内部缓冲区，下次请求会覆盖内容
 */
const char *esp_at_http_get(const char *url);

#endif /* __ESP_AT_H__ */
