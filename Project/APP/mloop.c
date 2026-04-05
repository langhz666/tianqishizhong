/**
 * @file mloop.c
 * @brief ��ѭ���������ģ��
 * 
 * ���ļ�ʵ�������������ն˵���ѭ���������ϵͳ�����÷�����ʽ
 * ʱ��Ƭ��ѯ�ܹ���������Ԥ��������ִ�У�����������
 * 
 * ������ȱ���
 * | ��������       | ִ�м�� | ��������                    |
 * |----------------|----------|-----------------------------|
 * | time_sync      | 10��     | SNTP����ʱ��ͬ��            |
 * | wifi_update    | 5��      | WiFi����״̬���            |
 * | time_update    | 1��      | RTCʱ���ȡ��UIˢ��         |
 * | inner_update   | 3��      | DHT11��ʪ�ȶ�ȡ��UIˢ��     |
 * | outdoor_update | 1����    | �����������ݻ�ȡ��UIˢ��    |
 * 
 * �ܹ��ص㣺
 * - ����HAL_GetTick()�ķ�������ʱ
 * - ���ݱ仯��⣬�����ظ�ˢ��UI
 * - ģ�黯��ƣ�������չ
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
/*                             ʱ�䵥λ�궨��                                  */
/*============================================================================*/

#define MILLISECONDS(x) (x)              /**< ����ת���� */
#define SECONDS(x)      ((x) * 1000)     /**< ��ת���� (���ں���) */
#define MINUTES(x)      (SECONDS(x) * 60)/**< ����ת���� */
#define HOURS(x)        (MINUTES(x) * 60)/**< Сʱת���� */
#define DAYS(x)         (HOURS(x) * 24)  /**< ��ת���� */

/*============================================================================*/
/*                             ����������                                    */
/*============================================================================*/

#define TIME_SYNC_INTERVAL          SECONDS(10)    /**< SNTPʱ��ͬ�����: 10�� */
#define WIFI_UPDATE_INTERVAL        SECONDS(5)     /**< WiFi״̬�����: 5�� */
#define TIME_UPDATE_INTERVAL        SECONDS(1)     /**< ʱ����ʾˢ�¼��: 1�� */
#define INNER_UPDATE_INTERVAL       SECONDS(3)     /**< ������ʪ��ˢ�¼��: 3�� */
#define OUTDOOR_UPDATE_INTERVAL     MINUTES(1)     /**< ��������ˢ�¼��: 1���� */

/*============================================================================*/
/*                             ����ʱ�������                                  */
/*============================================================================*/

static uint32_t last_time_sync_tick = 0;      /**< �ϴ�SNTPͬ��ʱ��� */
static uint32_t last_wifi_update_tick = 0;    /**< �ϴ�WiFi���ʱ��� */
static uint32_t last_time_update_tick = 0;    /**< �ϴ�ʱ��ˢ��ʱ��� */
static uint32_t last_inner_update_tick = 0;   /**< �ϴ���������ˢ��ʱ��� */
static uint32_t last_outdoor_update_tick = 0; /**< �ϴ���������ˢ��ʱ��� */

/*============================================================================*/
/*                             ˽�к�������                                    */
/*============================================================================*/

static void time_sync(void);
static void wifi_update(void);
static void time_update(void);
static void inner_update(void);
static void outdoor_update(void);

/*============================================================================*/
/*                             ��ʼ������                                      */
/*============================================================================*/

/**
 * @brief ��ѭ����ʼ��
 * 
 * ִ��ϵͳ�������̣�
 * 1. ��ʾ�������沢���½���
 * 2. ��ʼ��ESP WiFiģ��
 * 3. ����WiFi����
 * 4. ͬ��SNTPʱ��
 * 5. ��ʾ��ҳ��
 * 
 * ����������ʾ��
 * - 10%: ��ʼ��ʼ��
 * - 20%: WiFiģ�������
 * - 50%: ������
 * - 80%: ��������
 * - 100%: ��ӭ����
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
/*                             ����ʵ�ֺ���                                    */
/*============================================================================*/

/**
 * @brief SNTP����ʱ��ͬ������
 * 
 * ��NTP��������ȡ����ʱ�䣬��ͬ����STM32��RTCģ�顣
 * ͬ���ɹ���ǿ��ˢ��ʱ����ʾ��
 * 
 * ��������
 * - ͬ��ʧ�ܺ�1������
 * - ���С��2000��Ϊ��Ч����
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
 * @brief WiFi����״̬�������
 * 
 * ���ڼ��WiFi����״̬��״̬�仯ʱ����UI��ʾ��
 * ʹ�þ�̬���������ϴ�״̬�������ظ�ˢ�¡�
 * 
 * ״̬�仯������
 * - ���ӳɹ�����ʾSSID
 * - ���ӶϿ�����ʾ"wifi lost"
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
        main_page_redraw_wifi_ssid(info.ssid, true);
    }
    else
    {
        printf("[WIFI] disconnected from %s\n", last_info.ssid);
        main_page_redraw_wifi_ssid("wifi lost", false);
    }
    
    memcpy(&last_info, &info, sizeof(esp_wifi_info_t));
}

/**
 * @brief ʱ����ʾˢ������
 * 
 * ��STM32 RTC��ȡ��ǰʱ�䣬����UI��ʾ��
 * ÿ��ִ��һ�Σ�ʱ��仯ʱ��ˢ�½��档
 * 
 * ������Ч�Լ�飺
 * - ���С��2020��ΪRTCδ��ʼ��������ˢ��
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
 * @brief ������ʪ������ˢ������
 * 
 * ��DHT11��������ȡ��ʪ�����ݣ�����UI��ʾ��
 * ���ݱ仯ʱ��ˢ�½��棬������˸��
 * 
 * ��������DHT11
 * - �¶ȷ�Χ��0-50��C
 * - ʪ�ȷ�Χ��20-90%RH
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
 * @brief ������������ˢ������
 * 
 * ͨ��HTTP�����ȡ��֪����API���ݣ����������UI��ʾ��
 * ����WiFi����״̬��ִ������
 * 
 * API�ṩ�̣���֪���� (Seniverse)
 * ���ݸ���Ƶ�ʣ�ÿ����
 * 
 * ��������
 * - WiFiδ���ӣ���������
 * - HTTP����ʧ�ܣ���ӡ��־
 * - JSON����ʧ�ܣ���ӡ��־
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
/*                             ��ѭ�����                                      */
/*============================================================================*/

/**
 * @brief ��ѭ��ִ�к���
 * 
 * ��main������while(1)ѭ���е��ã�����ִ����������
 * �������ڲ�ʵ�ַ�������ʱ�жϣ�����������ִ�С�
 * 
 * ����˳�򣨰����ȼ����У���
 * 1. time_sync()    - ʱ��ͬ��
 * 2. wifi_update()  - WiFi״̬
 * 3. time_update()  - ʱ����ʾ
 * 4. inner_update() - ������ʪ��
 * 5. outdoor_update()- ��������
 */
void main_loop(void)
{
    time_sync();
    wifi_update();
    time_update();
    inner_update();
    outdoor_update();
}
