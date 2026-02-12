/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-02-11 16:24:31
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-02-12 10:39:54
 * @FilePath: \luoji\APP\page\wifi_page.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdint.h>
#include <string.h>
#include "lcd.h"
#include "lcdfont.h"
#include "imag.h"
#include "app.h"
#include "font.h"


#define COLOR_WIFI_TEXT    0x07FD 
#define COLOR_CONNECTING   0x965F

void wifi_page_display(void)
{
    static const char *ssid = WIFI_SSID;
    g_back_color = BLACK;
    lcd_fill(0, 0, lcddev.width - 1, lcddev.height - 1, BLACK);
    lcd_show_picture(30, 15, img_wifi.width, img_wifi.height, img_wifi.data);
    lcd_show_string(88, 191, 200, 32, 32, "WiFi", COLOR_WIFI_TEXT);
    uint16_t ssid_len_px = strlen(ssid) * 12; 
    uint16_t ssid_startx = 0;
    if (ssid_len_px < lcddev.width)
    {
        ssid_startx = (lcddev.width - ssid_len_px) / 2;
    }
    lcd_show_string(ssid_startx, 231, lcddev.width, 24, 24, (char *)ssid, WHITE);
    lcd_show_chinese(84, 263, (uint8_t *)"连接中", WHITE, BLACK, &font24_maple_bold);
}
