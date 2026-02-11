#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "bsp_rtc.h"
#include "lcd.h"
#include "lcdfont.h"
#include "imag.h"
#include "page.h"
#include "app.h"
#include "font.h"

// 颜色转换 (RGB888 -> RGB565)
// 如果你没有 mkcolor 宏，可以使用以下公式或在线工具转换
// 这里直接定义转换后的近似值
#define COLOR_BG_TIME     0xF7DE // mkcolor(248, 248, 248) -> White-ish
#define COLOR_BG_INNER    0x86D5 // mkcolor(136, 217, 234) -> Light Blue
#define COLOR_BG_OUTDOOR  0xFD49 // mkcolor(254, 135, 75)  -> Orange

// 注意：请根据 imag.h 中实际图片的宽高修改以下宏
#define ICON_WIFI_W      20 
#define ICON_WIFI_H      20
#define ICON_TEMP_W      20
#define ICON_TEMP_H      40
#define ICON_WEATHER_W   40
#define ICON_WEATHER_H   40

// 声明外部变量或函数（如果不在头文件中）
// void main_page_redraw_wifi_ssid(const char *ssid);
// void main_page_redraw_inner_temperature(float temperature);
// void main_page_redraw_inner_humidity(float humidity);
// void main_page_redraw_outdoor_city(const char *city);
// void main_page_redraw_outdoor_temperature(float temperature);
// void main_page_redraw_outdoor_weather_icon(int code);

// 假设 WIFI_SSID 定义在某处
#ifndef WIFI_SSID
#define WIFI_SSID "Connecting..."
#endif

void main_page_display(void)
{
    // 1. 全屏黑色背景
    lcd_clear(BLACK);
    
    // 2. 时间区域 (顶部白色区域)
    lcd_fill(15, 15, 224, 154, COLOR_BG_TIME);
    
    // Wifi 图标 (注意：需确认 icon_wifi 是数组还是结构体，这里假设是数组)
    lcd_show_picture(23, 20, ICON_WIFI_W, ICON_WIFI_H, (const uint8_t *)&icon_wifi);
    
    // 显示 WiFi SSID
    main_page_redraw_wifi_ssid(WIFI_SSID);
    
    // 时间初始显示 (原代码用76号字体，这里降级为32号)
    g_back_color = COLOR_BG_TIME; // 设置文字背景色
    lcd_show_string(25, 42, 200, 32, 32, "--:--", BLACK);
    
    // 日期初始显示
    lcd_show_string(35, 121, 200, 24, 24, "----/--/--", GRAY); // 星期几是中文，需要单独处理或扩展字库
    
    // 3. 室内环境区域 (左下蓝色区域)
    lcd_fill(15, 165, 114, 304, COLOR_BG_INNER);
    
    // 显示中文 "室内环境"
    g_back_color = COLOR_BG_INNER;
    // 注意：正点原子 lcd_show_chinese 需要字库支持，且通常只支持特定大小(16/24/32)
    // 这里假设使用 24号字体显示中文
    lcd_show_chinese(19, 170, (uint8_t *)"室内环境", BLACK, COLOR_BG_INNER, FONT_SIZE_24); 
    
    lcd_show_string(86, 191, 20, 32, 32, "C", BLACK);
    lcd_show_string(91, 262, 20, 32, 32, "%", BLACK);
    
    main_page_redraw_inner_temperature(99.9f); // 初始值
    main_page_redraw_inner_humidity(99.9f);    // 初始值
    
    // 4. 室外环境区域 (右下橙色区域)
    lcd_fill(125, 165, 224, 304, COLOR_BG_OUTDOOR);
    
    g_back_color = COLOR_BG_OUTDOOR;
    lcd_show_string(192, 189, 20, 32, 32, "C", BLACK);
    
    // 温度计图标
    lcd_show_picture(139, 239, ICON_TEMP_W, ICON_TEMP_H, (const uint8_t *)&icon_wenduji);
    
    main_page_redraw_outdoor_city("合肥"); // 假设 city 是纯汉字，需特殊处理
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
    // 假设 lcd_show_chinese 支持 "星期" 和 数字对应的汉字
    // 坐标需要手动计算偏移：10个字符 * 12像素 = 120像素偏移
    lcd_show_chinese(35 + 120, 121, (uint8_t *)"星期", GRAY, COLOR_BG_TIME, FONT_SIZE_24);
    lcd_show_chinese(35 + 120 + 48, 121, (uint8_t *)week_str, GRAY, COLOR_BG_TIME, FONT_SIZE_24);
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
    // 假设 city 是中文
    lcd_show_chinese(127, 170, (uint8_t *)city, BLACK, COLOR_BG_OUTDOOR, FONT_SIZE_24);
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
    const uint8_t *icon; // 假设图片是 uint8_t 数组
    
    // 这里需要根据 imag.h 的定义，如果是结构体请改为结构体指针
    if (code == 0 || code == 2 || code == 38)
        icon = (const uint8_t *)&icon_qing;
    else if (code == 1 || code == 3)
        icon = (const uint8_t *)&icon_yueliang;
    else if (code == 4 || code == 9)
        icon = (const uint8_t *)&icon_yintian;
    else if (code == 5 || code == 6 || code == 7 || code == 8)
        icon = (const uint8_t *)&icon_duoyun;
    else if (code == 10 || code == 13 || code == 14 || code == 15 || code == 16 || code == 17 || code == 18 || code == 19)
        icon = (const uint8_t *)&icon_zhongyu;
    else if (code == 11 || code == 12)
        icon = (const uint8_t *)&icon_leizhenyu;
    else if (code == 20 || code == 21 || code == 22 || code == 23 || code == 24 || code == 25)
        icon = (const uint8_t *)&icon_zhongxue;
    else 
        icon = (const uint8_t *)&icon_na;
        
    // 显示图片，务必确认 ICON_WEATHER_W/H 与实际图片尺寸一致
    lcd_show_picture(166, 240, ICON_WEATHER_W, ICON_WEATHER_H, icon);
}
