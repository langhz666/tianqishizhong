#include <stdint.h>
#include <string.h>
#include "lcd.h"
#include "lcdfont.h"
#include "imag.h"
#include "app.h"
#include "font.h"


#define COLOR_WIFI_TEXT    0x07FD 
#define COLOR_CONNECTING   0x965F
#define IMG_WIFI_W  180
#define IMG_WIFI_H  160

void wifi_page_display(void)
{
    static const char *ssid = WIFI_SSID;
    g_back_color = BLACK;
    lcd_clear(BLACK);
    lcd_show_picture(30, 15, IMG_WIFI_W, IMG_WIFI_H, (const uint8_t *)&img_wifi);
    lcd_show_string(88, 191, 200, 32, 32, "WiFi", COLOR_WIFI_TEXT);
    uint16_t ssid_len_px = strlen(ssid) * 12; 
    uint16_t ssid_startx = 0;
    if (ssid_len_px < lcddev.width)
    {
        ssid_startx = (lcddev.width - ssid_len_px) / 2;
    }
    lcd_show_string(ssid_startx, 231, lcddev.width, 24, 24, (char *)ssid, WHITE);
    lcd_show_chinese(84, 263, (uint8_t *)"Á¬½ÓÖÐ", WHITE, BLACK, &font24_maple_bold);
}
