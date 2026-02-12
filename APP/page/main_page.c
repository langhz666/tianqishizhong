#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "bsp_rtc.h"
#include "lcd.h"
#include "lcdfont.h"
#include "imag.h"
#include "page.h"
#include "app.h"
#include "font.h"

#define COLOR_BG_TIME     0xF7DE // mkcolor(248, 248, 248) -> White-ish
#define COLOR_BG_INNER    0x86D5 // mkcolor(136, 217, 234) -> Light Blue
#define COLOR_BG_OUTDOOR  0xFD49 // mkcolor(254, 135, 75)  -> Orange



#ifndef WIFI_SSID
#define WIFI_SSID "Connecting..."
#endif

void main_page_display(void)
{
    lcd_clear(BLACK);
    lcd_fill(15, 15, 224, 154, COLOR_BG_TIME);
    lcd_show_picture(23, 20, icon_wifi.width, icon_wifi.height, icon_wifi.data);
    main_page_redraw_wifi_ssid(WIFI_SSID);
    g_back_color = COLOR_BG_TIME; 
    lcd_show_string(25, 42, 200, 76, 76, "--:--", BLACK);
    lcd_show_string(35, 121, 200, 20, 20, "----/--/--", BLACK); 
    lcd_show_chinese(140, 121, (uint8_t *)"星期四", BLACK, COLOR_BG_TIME, &font20_maple_bold);

    lcd_fill(15, 165, 114, 304, COLOR_BG_INNER);
    g_back_color = COLOR_BG_INNER;
    lcd_show_chinese(19, 170, (uint8_t *)"室内环境", BLACK, COLOR_BG_INNER, &font24_maple_bold);
    lcd_show_string(86, 191, 20, 32, 32, "C", BLACK);
    lcd_show_string(91, 262, 20, 32, 32, "%", BLACK);
    main_page_redraw_inner_temperature(999.9f); 
    main_page_redraw_inner_humidity(999.9f);    
    lcd_fill(125, 165, 224, 304, COLOR_BG_OUTDOOR);
    g_back_color = COLOR_BG_OUTDOOR;
    lcd_show_string(192, 189, 20, 32, 32, "C", BLACK);
    lcd_show_picture(139, 239, icon_wenduji.width, icon_wenduji.height, icon_wenduji.data);
    main_page_redraw_outdoor_city("衡阳"); 
    main_page_redraw_outdoor_temperature(99.9f);
    main_page_redraw_outdoor_weather_icon(-1);
}

void main_page_redraw_wifi_ssid(const char *ssid)
{
    g_back_color = COLOR_BG_TIME;
    lcd_show_string(50, 23, 160, 16, 16, (char *)ssid, GRAY);
}

void main_page_redraw_time(rtc_date_time_t *time)
{
    char str[9];
    char comma = (time->second % 2 == 0) ? ':' : ' ';
    snprintf(str, sizeof(str), "%02u%c%02u", time->hour, comma, time->minute);
    g_back_color = COLOR_BG_TIME;
    lcd_show_string(25, 42, 200, 76, 76, str, BLACK);
}

void main_page_redraw_date(rtc_date_time_t *date)
{
    char str_date[16];
    char *week_str;
    
    snprintf(str_date, sizeof(str_date), "%04u/%02u/%02u", date->year, date->month, date->day);
    
    switch(date->weekday) {
        case 1: week_str = "一"; break;
        case 2: week_str = "二"; break;
        case 3: week_str = "三"; break;
        case 4: week_str = "四"; break;
        case 5: week_str = "五"; break;
        case 6: week_str = "六"; break;
        case 7: week_str = "日"; break; // 注意原代码是"天"
        default: week_str = "X"; break;
    }

    g_back_color = COLOR_BG_TIME;
    lcd_show_string(35, 121, 120, 24, 24, str_date, GRAY);
    lcd_show_chinese(35 + 120, 121, (uint8_t *)"星期", GRAY, COLOR_BG_TIME, &font24_maple_bold);
    lcd_show_chinese(35 + 120 + 48, 121, (uint8_t *)week_str, GRAY, COLOR_BG_TIME, &font24_maple_bold);
}

void main_page_redraw_inner_temperature(float temperature)
{
    char str[4] = "--"; 
    if (temperature > -10.0f && temperature <= 100.0f)
        snprintf(str, sizeof(str), "%2.0f", temperature);
    g_back_color = COLOR_BG_INNER;
    lcd_show_string(30, 192, 80, 54, 54, str, BLACK);
}
    
void main_page_redraw_inner_humidity(float humidity)
{
    char str[4] = "--"; 
    if (humidity > 0.0f && humidity <= 99.99f)
        snprintf(str, sizeof(str), "%2.0f", humidity);  
    g_back_color = COLOR_BG_INNER;
    lcd_show_string(25, 239, 80, 64, 64, str, BLACK);
}

void main_page_redraw_outdoor_city(const char *city)
{
    char str[9];
    snprintf(str, sizeof(str), "%s", city);
    g_back_color = COLOR_BG_OUTDOOR;
    lcd_show_chinese(127, 170, (uint8_t *)str, BLACK, COLOR_BG_OUTDOOR, &font24_maple_bold);
}

void main_page_redraw_outdoor_temperature(float temperature)
{
    char str[4] = "--"; 
    if (temperature > -10.0f && temperature <= 100.0f)
        snprintf(str, sizeof(str), "%2.0f", temperature);
    g_back_color = COLOR_BG_OUTDOOR;
    lcd_show_string(135, 190, 80, 54, 54, str, BLACK);
}

void main_page_redraw_outdoor_weather_icon(const int code)
{
    const image_t *icon; 
    if (code == 0 || code == 2 || code == 38)
        icon = &icon_qing;
    else if (code == 1 || code == 3)
        icon = &icon_yueliang;
    else if (code == 4 || code == 9)
        icon = &icon_yintian;
    else if (code == 5 || code == 6 || code == 7 || code == 8)
        icon = &icon_duoyun;
    else if (code == 10 || code == 13 || code == 14 || code == 15 || code == 16 || code == 17 || code == 18 || code == 19)
        icon = &icon_zhongyu;
    else if (code == 11 || code == 12)
        icon = &icon_leizhenyu;
    else if (code == 20 || code == 21 || code == 22 || code == 23 || code == 24 || code == 25)
        icon = &icon_zhongxue;
    else 
        icon = &icon_na;
    lcd_show_picture(166, 240, icon->width, icon->height, icon->data);
}
