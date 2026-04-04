/**
 * @file mloop.c
 * @brief 主循环任务调度模块
 * 
 * 本文件实现了智能桌面终端的主循环任务调度系统，采用非阻塞式
 * 时间片轮询架构，各任务按预设间隔独立执行，互不阻塞。
 * 
 * 任务调度表：
 * | 任务名称       | 执行间隔 | 功能描述                    |
 * |----------------|----------|-----------------------------|
 * | time_sync      | 10秒     | SNTP网络时间同步            |
 * | wifi_update    | 5秒      | WiFi连接状态检测            |
 * | time_update    | 1秒      | RTC时间读取与UI刷新         |
 * | inner_update   | 3秒      | DHT11温湿度读取与UI刷新     |
 * | outdoor_update | 1分钟    | 网络天气数据获取与UI刷新    |
 * 
 * 架构特点：
 * - 基于HAL_GetTick()的非阻塞延时
 * - 数据变化检测，避免重复刷新UI
 * - 模块化设计，易于扩展
 * 
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "bsp_rtc.h"
#include "bsp_espat.h"
#include "bsp_dht11.h"

#include "weather.h"
#include "lcd.h"
#include "page.h"
#include "app.h"

/*============================================================================*/
/*                             时间单位宏定义                                  */
/*============================================================================*/

#define MILLISECONDS(x) (x)              /**< 毫秒转换宏 */
#define SECONDS(x)      ((x) * 1000)     /**< 秒转换宏 (基于毫秒) */
#define MINUTES(x)      (SECONDS(x) * 60)/**< 分钟转换宏 */
#define HOURS(x)        (MINUTES(x) * 60)/**< 小时转换宏 */
#define DAYS(x)         (HOURS(x) * 24)  /**< 天转换宏 */

/*============================================================================*/
/*                             任务间隔配置                                    */
/*============================================================================*/

#define TIME_SYNC_INTERVAL          SECONDS(10)    /**< SNTP时间同步间隔: 10秒 */
#define WIFI_UPDATE_INTERVAL        SECONDS(5)     /**< WiFi状态检测间隔: 5秒 */
#define TIME_UPDATE_INTERVAL        SECONDS(1)     /**< 时间显示刷新间隔: 1秒 */
#define INNER_UPDATE_INTERVAL       SECONDS(3)     /**< 室内温湿度刷新间隔: 3秒 */
#define OUTDOOR_UPDATE_INTERVAL     MINUTES(1)     /**< 室外天气刷新间隔: 1分钟 */

/*============================================================================*/
/*                             任务时间戳变量                                  */
/*============================================================================*/

static uint32_t last_time_sync_tick = 0;      /**< 上次SNTP同步时间戳 */
static uint32_t last_wifi_update_tick = 0;    /**< 上次WiFi检测时间戳 */
static uint32_t last_time_update_tick = 0;    /**< 上次时间刷新时间戳 */
static uint32_t last_inner_update_tick = 0;   /**< 上次室内数据刷新时间戳 */
static uint32_t last_outdoor_update_tick = 0; /**< 上次室外数据刷新时间戳 */

/*============================================================================*/
/*                             私有函数声明                                    */
/*============================================================================*/

static void time_sync(void);
static void wifi_update(void);
static void time_update(void);
static void inner_update(void);
static void outdoor_update(void);

/*============================================================================*/
/*                             初始化函数                                      */
/*============================================================================*/

/**
 * @brief 主循环初始化
 * 
 * 执行系统启动流程：
 * 1. 显示启动画面并更新进度
 * 2. 初始化ESP WiFi模块
 * 3. 连接WiFi网络
 * 4. 同步SNTP时间
 * 5. 显示主页面
 * 
 * 启动进度显示：
 * - 10%: 开始初始化
 * - 20%: WiFi模块检测完成
 * - 50%: 加载中
 * - 80%: 即将就绪
 * - 100%: 欢迎界面
 */
void main_loop_init(void)
{
    splash_set_progress(10, "Initializing...");
    HAL_Delay(200);
    
    printf("[SYS] ESP Init...\n");
    if (!esp_at_init()) {
        printf("[ERR] ESP Init failed\n");
        splash_set_progress(20, "WiFi init failed");
    } else {
        splash_set_progress(20, "WiFi module OK");
    }
    HAL_Delay(300);
    
    splash_set_progress(50, "Loading...");
    HAL_Delay(300);
    
    splash_set_progress(80, "Almost ready...");
    HAL_Delay(200);
    
    splash_set_progress(100, "Welcome!");
    HAL_Delay(500);
    
    splash_screen_end();
    
    wifi_page_display();
    
    printf("[SYS] Connecting WiFi...\n");
    wifi_page_set_status("Connecting...", false);
    HAL_Delay(500);
    
    if (esp_at_connect_wifi("iQOO Neo8 Pro", "lhz19719937532", NULL)) {
        printf("[SYS] WiFi Connected\n");
        wifi_page_set_status("Connected!", true);
        HAL_Delay(1000);
    } else {
        printf("[ERR] WiFi Connect failed\n");
        wifi_page_set_status("Connect failed", false);
        HAL_Delay(2000);
    }
    
    wifi_page_set_status("Syncing time...", true);
    esp_at_sntp_init();
    HAL_Delay(500);
    
    main_page_display();
    
    last_time_sync_tick = 0;
    last_wifi_update_tick = 0;
    last_time_update_tick = 0;
    last_inner_update_tick = 0;
    last_outdoor_update_tick = 0;
    
    time_sync();
}

/*============================================================================*/
/*                             任务实现函数                                    */
/*============================================================================*/

/**
 * @brief SNTP网络时间同步任务
 * 
 * 从NTP服务器获取网络时间，并同步到STM32的RTC模块。
 * 同步成功后强制刷新时间显示。
 * 
 * 错误处理：
 * - 同步失败后1秒重试
 * - 年份小于2000视为无效数据
 */
static void time_sync(void)
{
    if (HAL_GetTick() - last_time_sync_tick < TIME_SYNC_INTERVAL)
        return;
    
    last_time_sync_tick = HAL_GetTick();
    
    esp_date_time_t esp_date = { 0 };
    if (!esp_at_sntp_get_time(&esp_date))
    {
        printf("[SNTP] get time failed\n");
        last_time_sync_tick = HAL_GetTick() - TIME_SYNC_INTERVAL + SECONDS(1);
        return;
    }
    
    if (esp_date.year < 2000)
    {
        printf("[SNTP] invalid date formate\n");
        last_time_sync_tick = HAL_GetTick() - TIME_SYNC_INTERVAL + SECONDS(1);
        return;
    }
    
    printf("[SNTP] sync time: %04u-%02u-%02u %02u:%02u:%02u (%d)\n",
        esp_date.year, esp_date.month, esp_date.day,
        esp_date.hour, esp_date.minute, esp_date.second, esp_date.weekday);
    
    rtc_date_time_t rtc_date = { 0 };
    rtc_date.year = esp_date.year;
    rtc_date.month = esp_date.month;
    rtc_date.day = esp_date.day;
    rtc_date.hour = esp_date.hour;
    rtc_date.minute = esp_date.minute;
    rtc_date.second = esp_date.second;
    rtc_date.weekday = esp_date.weekday;
    
    bsp_rtc_set_time(&rtc_date);
    
    last_time_update_tick = 0;
}

/**
 * @brief WiFi连接状态检测任务
 * 
 * 定期检测WiFi连接状态，状态变化时更新UI显示。
 * 使用静态变量缓存上次状态，避免重复刷新。
 * 
 * 状态变化处理：
 * - 连接成功：显示SSID
 * - 连接断开：显示"wifi lost"
 */
static void wifi_update(void)
{
    static esp_wifi_info_t last_info = { 0 };

    if (HAL_GetTick() - last_wifi_update_tick < WIFI_UPDATE_INTERVAL)
        return;
    
    last_wifi_update_tick = HAL_GetTick();
    
    esp_wifi_info_t info = { 0 };
    if (!esp_at_get_wifi_info(&info))
    {
        printf("[AT] wifi info get failed\n");
        return;
    }
    
    if (memcmp(&info, &last_info, sizeof(esp_wifi_info_t)) == 0)
    {
        return;
    }
    
    if (last_info.connected == info.connected)
    {
        return;
    }
    
    if (info.connected)
    {
        printf("[WIFI] connected to %s\n", info.ssid);
        printf("[WIFI] SSID: %s, BSSID: %s, Channel: %d, RSSI: %d\n",
                info.ssid, info.bssid, info.channel, info.rssi);
        main_page_redraw_wifi_ssid(info.ssid);
    }
    else
    {
        printf("[WIFI] disconnected from %s\n", last_info.ssid);
        main_page_redraw_wifi_ssid("wifi lost");
    }
    
    memcpy(&last_info, &info, sizeof(esp_wifi_info_t));
}

/**
 * @brief 时间显示刷新任务
 * 
 * 从STM32 RTC读取当前时间，更新UI显示。
 * 每秒执行一次，时间变化时才刷新界面。
 * 
 * 数据有效性检查：
 * - 年份小于2020视为RTC未初始化，跳过刷新
 */
static void time_update(void)
{
    static rtc_date_time_t last_date = { 0 };
    
    if (HAL_GetTick() - last_time_update_tick < TIME_UPDATE_INTERVAL)
        return;
    
    last_time_update_tick = HAL_GetTick();
    
    rtc_date_time_t date;
    bsp_rtc_get_time(&date);
    
    if (date.year < 2020)
    {
        return;
    }
    
    if (memcmp(&date, &last_date, sizeof(rtc_date_time_t)) == 0)
    {
        return;
    }
    
    memcpy(&last_date, &date, sizeof(rtc_date_time_t));
    main_page_redraw_time(&date);
    main_page_redraw_date(&date);
}

/**
 * @brief 室内温湿度数据刷新任务
 * 
 * 从DHT11传感器读取温湿度数据，更新UI显示。
 * 数据变化时才刷新界面，避免闪烁。
 * 
 * 传感器：DHT11
 * - 温度范围：0-50°C
 * - 湿度范围：20-90%RH
 */
static void inner_update(void)
{
    static uint8_t last_temperature, last_humidity;
    
    if (HAL_GetTick() - last_inner_update_tick < INNER_UPDATE_INTERVAL)
        return;
    
    last_inner_update_tick = HAL_GetTick();
    
    uint8_t temperature = 0, humidity = 0;
    
    if (dht11_read_data(&temperature, &humidity) != 0)
    {
        printf("[DHT11] read data failed\n");
        return;
    }

    if (temperature == last_temperature && humidity == last_humidity)
    {
        return;
    }

    last_temperature = temperature;
    last_humidity = humidity;

    main_page_redraw_inner_temperature((float)temperature);
    main_page_redraw_inner_humidity((float)humidity);
}

/**
 * @brief 室外天气数据刷新任务
 * 
 * 通过HTTP请求获取心知天气API数据，解析后更新UI显示。
 * 仅在WiFi连接状态下执行请求。
 * 
 * API提供商：心知天气 (Seniverse)
 * 数据更新频率：每分钟
 * 
 * 错误处理：
 * - WiFi未连接：跳过请求
 * - HTTP请求失败：打印日志
 * - JSON解析失败：打印日志
 */
static void outdoor_update(void)
{
    static weather_info_t last_weather = { 0 };
    
    if (HAL_GetTick() - last_outdoor_update_tick < OUTDOOR_UPDATE_INTERVAL)
        return;
    
    if (!wifi_is_connected()) { 
        return; 
    }

    last_outdoor_update_tick = HAL_GetTick();
    
    weather_info_t weather = { 0 };
    
    static const char *weather_url = "http://api.seniverse.com/v3/weather/daily.json?key=SMrYk_pYNmh3z37k5&location=Hengyang&language=en&unit=c&days=1";

    printf("[WEATHER] requesting weather data...\n");
    const char *weather_http_response = esp_at_http_get(weather_url);
    
    if (weather_http_response == NULL)
    {
        printf("[WEATHER] http error - no response\n");
        return;
    }

    printf("[WEATHER] response received, parsing...\n");
    
    const char *json_start = strchr(weather_http_response, '{');
    if (json_start == NULL) {
        printf("[WEATHER] invalid response (no json)\n");
        return;
    }

    if (!parse_seniverse_response(json_start, &weather))
    {
        printf("[WEATHER] parse failed\n");
        return;
    }
    
    if (memcmp(&last_weather, &weather, sizeof(weather_info_t)) == 0)
    {
        printf("[WEATHER] data unchanged, skip update\n");
        return;
    }
    
    memcpy(&last_weather, &weather, sizeof(weather_info_t));
    printf("[WEATHER] %s, %s, %d, code: %d\n", weather.city, weather.weather, weather.temperature, weather.weather_code);
    printf("[WEATHER] humidity: %d%%, wind: %s %d km/h\n", weather.humidity, weather.wind_direction, weather.wind_speed);
    printf("[WEATHER] calling redraw functions\n");
    
    main_page_redraw_outdoor_temperature(weather.temperature);
    main_page_redraw_outdoor_weather_icon(weather.weather_code);
    main_page_redraw_outdoor_humidity(weather.humidity);
    main_page_redraw_outdoor_wind(weather.wind_speed, weather.wind_direction);
}

/*============================================================================*/
/*                             主循环入口                                      */
/*============================================================================*/

/**
 * @brief 主循环执行函数
 * 
 * 在main函数的while(1)循环中调用，依次执行所有任务。
 * 各任务内部实现非阻塞延时判断，满足条件才执行。
 * 
 * 调用顺序（按优先级排列）：
 * 1. time_sync()    - 时间同步
 * 2. wifi_update()  - WiFi状态
 * 3. time_update()  - 时间显示
 * 4. inner_update() - 室内温湿度
 * 5. outdoor_update()- 室外天气
 */
void main_loop(void)
{
    time_sync();
    wifi_update();
    time_update();
    inner_update();
    outdoor_update();
}
