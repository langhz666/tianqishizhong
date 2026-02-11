#include <stdint.h>
#include <string.h>
#include "lcd.h"
#include "lcdfont.h"
#include "imag.h"
#include "app.h"
#include "font.h"

// 1. ???? (RGB888 -> RGB565)
// 0, 255, 234 (??) -> 0x07FD
#define COLOR_WIFI_TEXT    0x07FD 
// 148, 198, 255 (??) -> 0x965F
#define COLOR_CONNECTING   0x965F

// 2. ???? (?????? img_wifi ?????)
// ????(30,15)???????(191)??????????150??
#define IMG_WIFI_W  180
#define IMG_WIFI_H  160

void wifi_page_display(void)
{
    // ?? SSID
    static const char *ssid = WIFI_SSID;
    
    // 1. ??????? (?? lcd_show_string ?????)
    g_back_color = BLACK;
    
    // 2. ????
    // ?? st7789_fill_color(..., mkcolor(0,0,0))
    lcd_clear(BLACK);
    
    // 3. ?? WiFi ??
    // ?? st7789_draw_image(30, 15, &img_wifi);
    lcd_show_picture(30, 15, IMG_WIFI_W, IMG_WIFI_H, (const uint8_t *)&img_wifi);
    
    // 4. ?? "WiFi" ?? (32???)
    // ?? st7789_write_string(88, 191, "WiFi", ... font32 ...);
    lcd_show_string(88, 191, 200, 32, 32, "WiFi", COLOR_WIFI_TEXT);
    
    // 5. ?? SSID (????)
    // ?????? font20 (??10)????? font24 (??12)
    // ??????
    uint16_t ssid_len_px = strlen(ssid) * 12; // 24????????12
    uint16_t ssid_startx = 0;
    
    if (ssid_len_px < lcddev.width)
    {
        ssid_startx = (lcddev.width - ssid_len_px) / 2;
    }
    
    // ?? SSID????????24?
    lcd_show_string(ssid_startx, 231, lcddev.width, 24, 24, (char *)ssid, WHITE);
    
    // 6. ?? "???" (?? 24???)
    lcd_show_chinese(84, 263, (uint8_t *)"???", COLOR_CONNECTING, BLACK, &font24_maple_bold);
}
