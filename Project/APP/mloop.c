#include "main.h"       // 引入 HAL 库定义 (HAL_GetTick)
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* BSP 头文件适配 */
#include "bsp_rtc.h"    // 替换 rtc.h
#include "bsp_espat.h"  // 替换 esp_at.h
#include "bsp_dht11.h"   // 替换 dht11.h

#include "weather.h"
#include "lcd.h"        // 引入 LCD 以便重绘
#include "page.h"
#include "app.h"

/* 1. 时间单位宏定义 (适配 HAL_GetTick 的毫秒基准) */
#define MILLISECONDS(x) (x)
#define SECONDS(x)      ((x) * 1000)
#define MINUTES(x)      (SECONDS(x) * 60)
#define HOURS(x)        (MINUTES(x) * 60)
#define DAYS(x)         (HOURS(x) * 24)

/* 2. 刷新间隔定义 */
#define TIME_SYNC_INTERVAL          SECONDS(10)
#define WIFI_UPDATE_INTERVAL        SECONDS(5)
#define TIME_UPDATE_INTERVAL        SECONDS(1)
#define INNER_UPDATE_INTERVAL       SECONDS(3)
#define OUTDOOR_UPDATE_INTERVAL     MINUTES(1)

/* 3. 上次执行的时间戳 (替代原先的 delay 计数器) */
static uint32_t last_time_sync_tick = 0;
static uint32_t last_wifi_update_tick = 0;
static uint32_t last_time_update_tick = 0;
static uint32_t last_inner_update_tick = 0;
static uint32_t last_outdoor_update_tick = 0;

static void time_sync(void);

/* ---------------- 核心初始化函数 ---------------- */

void main_loop_init(void)
{
    main_page_display();
    
    // 【新增 1】移植丢失的初始化逻辑
    printf("[SYS] ESP Init...\n");
    if (!esp_at_init()) {
        printf("[ERR] ESP Init failed\n");
    }

    // 【新增 2】移植丢失的连接 WiFi 逻辑
    // 注意：这里的 SSID 和密码根据你的实际情况填写，或者做成宏定义
    printf("[SYS] Connecting WiFi...\n");
    main_page_redraw_wifi_ssid("Connecting..."); // 可以在屏幕提示正在连接
    
    if (esp_at_connect_wifi("iQOO Neo8 Pro", "lhz19719937532", NULL)) {
        printf("[SYS] WiFi Connected\n");
        main_page_redraw_wifi_ssid("Connected");
    } else {
        printf("[ERR] WiFi Connect failed\n");
        main_page_redraw_wifi_ssid("Connect Fail");
    }

    // 【新增 3】SNTP 初始化
    esp_at_sntp_init();

    // 初始化时间戳
    last_time_sync_tick = 0;
    last_wifi_update_tick = 0;
    last_time_update_tick = 0;
    last_inner_update_tick = 0;
    last_outdoor_update_tick = 0;
    
    // 首次启动时立即同步网络时间
    time_sync();
}

/* ---------------- 内部静态任务函数 (保持原有名称) ---------------- */

static void time_sync(void)
{
    // 非阻塞延时判断：如果 (当前时间 - 上次时间) < 间隔，则退出
    if (HAL_GetTick() - last_time_sync_tick < TIME_SYNC_INTERVAL)
        return;
    
    // 更新执行时间
    last_time_sync_tick = HAL_GetTick();
    
    esp_date_time_t esp_date = { 0 };
    if (!esp_at_sntp_get_time(&esp_date))
    {
        printf("[SNTP] get time failed\n");
        // 失败重试逻辑：修改上次执行时间，使下次执行在 1秒后
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
    
    // 使用新的 BSP 接口
    bsp_rtc_set_time(&rtc_date);
    
    // 同步成功后，强制让 time_update 立即执行一次刷新屏幕
    last_time_update_tick = 0;
}

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

static void time_update(void)
{
    static rtc_date_time_t last_date = { 0 };
    
    if (HAL_GetTick() - last_time_update_tick < TIME_UPDATE_INTERVAL)
        return;
    
    last_time_update_tick = HAL_GetTick();
    
    rtc_date_time_t date;
    // 使用新的 BSP 接口
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

static void outdoor_update(void)
{
    static weather_info_t last_weather = { 0 };
    
    if (HAL_GetTick() - last_outdoor_update_tick < OUTDOOR_UPDATE_INTERVAL)
        return;
    
    // 检查 WiFi 是否连接，没连接就别请求了，省得卡住
    if (!wifi_is_connected()) { 
        return; 
    }

    last_outdoor_update_tick = HAL_GetTick();
    
    weather_info_t weather = { 0 };
    
    // 【优化 1】改为 http，提高成功率
    static const char *weather_url = "http://api.seniverse.com/v3/weather/now.json?key=SMrYk_pYNmh3z37k5&location=Hengyang&language=en&unit=c";

    printf("[WEATHER] requesting weather data...\n");
    const char *weather_http_response = esp_at_http_get(weather_url);
    
    if (weather_http_response == NULL)
    {
        printf("[WEATHER] http error - no response\n");
        return;
    }

    printf("[WEATHER] response received, parsing...\n");
    
    // 【优化 2】跳过 AT 指令头，寻找 JSON 起始点 '{'
    const char *json_start = strchr(weather_http_response, '{');
    if (json_start == NULL) {
        printf("[WEATHER] invalid response (no json)\n");
        return;
    }

    // 传入 json_start 而不是原始 response
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
    printf("[WEATHER] calling redraw functions\n");
    
    main_page_redraw_outdoor_temperature(weather.temperature);
    main_page_redraw_outdoor_weather_icon(weather.weather_code);
    // 如果 page.h 有重绘城市的函数，建议也加上
    // main_page_redraw_outdoor_city(weather.city);
}

/* ---------------- 主循环接口 ---------------- */

void main_loop(void)
{
    time_sync();
    wifi_update();
    time_update();
    inner_update();
    outdoor_update();
}
