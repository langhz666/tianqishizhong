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
    lcd_show_string(25, 42, 200, 32, 32, "--:--", BLACK);
    lcd_show_string(35, 121, 200, 24, 24, "----/--/--", GRAY); 

    lcd_fill(15, 165, 114, 304, COLOR_BG_INNER);
    g_back_color = COLOR_BG_INNER;
    lcd_show_chinese(19, 170, (uint8_t *)"室内环境", BLACK, COLOR_BG_INNER, &font24_maple_bold);
    lcd_show_string(86, 191, 20, 32, 32, "C", BLACK);
    lcd_show_string(91, 262, 20, 32, 32, "%", BLACK);
    main_page_redraw_inner_temperature(99.9f); 
    main_page_redraw_inner_humidity(99.9f);    
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
    // 限制长度防止溢出
    lcd_show_string(50, 23, 160, 16, 16, (char *)ssid, GRAY);
}

void main_page_redraw_time(rtc_date_time_t *time)
{
    char str[9];
    // 模拟闪烁效果
    char comma = (time->second % 2 == 0) ? ':' : ' ';
    snprintf(str, sizeof(str), "%02u%c%02u", time->hour, comma, time->minute);
    
    g_back_color = COLOR_BG_TIME;
    // 原代码 76 号字体，改为 32 号 (驱动限制)
    // 如果需要更大字体，需要扩展 lcd.c 中的 show_char 函数
    lcd_show_string(25, 42, 200, 32, 32, str, BLACK);
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
    // 显示日期部分 (英文/数字)
    lcd_show_string(35, 121, 120, 24, 24, str_date, GRAY);
    
    // 显示 "星期X" (中文)
    lcd_show_chinese(35 + 120, 121, (uint8_t *)"星期", GRAY, COLOR_BG_TIME, &font24_maple_bold);
    lcd_show_chinese(35 + 120 + 48, 121, (uint8_t *)week_str, GRAY, COLOR_BG_TIME, &font24_maple_bold);
}

void main_page_redraw_inner_temperature(float temperature)
{
    char str[10];
    if (temperature > -10.0f && temperature <= 100.0f)
        snprintf(str, sizeof(str), "%2.0f", temperature);
    else
        strcpy(str, "--");

    g_back_color = COLOR_BG_INNER;
    // 原 54 号 -> 改 32 号
    lcd_show_string(30, 192, 80, 32, 32, str, BLACK);
}
    
void main_page_redraw_inner_humidity(float humidity)
{
    char str[10];
    if (humidity > 0.0f && humidity <= 99.99f)
        snprintf(str, sizeof(str), "%2.0f", humidity);
    else
        strcpy(str, "--");
        
    g_back_color = COLOR_BG_INNER;
    // 原 64 号 -> 改 32 号
    lcd_show_string(25, 239, 80, 32, 32, str, BLACK);
}

void main_page_redraw_outdoor_city(const char *city)
{
    g_back_color = COLOR_BG_OUTDOOR;
    lcd_show_chinese(127, 170, (uint8_t *)city, BLACK, COLOR_BG_OUTDOOR, &font24_maple_bold);
}

void main_page_redraw_outdoor_temperature(float temperature)
{
    char str[10];
    if (temperature > -10.0f && temperature <= 100.0f)
        snprintf(str, sizeof(str), "%2.0f", temperature);
    else
        strcpy(str, "--");
        
    g_back_color = COLOR_BG_OUTDOOR;
    // 原 54 号 -> 改 32 号
    lcd_show_string(135, 190, 80, 32, 32, str, BLACK);
}

void main_page_redraw_outdoor_weather_icon(const int code)
{
    // 【修改点1】这里不能定义为 uint8_t*，必须定义为图片结构体的指针
    const image_t *icon; 
    
    // 【修改点2】去掉强制类型转换，直接赋值结构体的地址
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
        
    // 【修改点3】正确调用 lcd_show_picture
    // 1. 指针访问成员用 -> 而不是 .
    // 2. width 和 height 需要传递数值，不要加 &
    // 3. 图片数据直接传递 icon->data
    lcd_show_picture(166, 240, icon->width, icon->height, icon->data);
}
