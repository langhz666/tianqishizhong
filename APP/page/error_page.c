#include <stdint.h>
#include <string.h>
#include "lcd.h"
#include "lcdfont.h"
#include "imag.h"
#include "font.h"

void error_page_display(const char *msg)
{

    g_back_color = BLACK; 
    lcd_clear(BLACK);
    lcd_show_picture(40, 37, img_error.width, img_error.height, img_error.data);
    uint8_t font_size = 24; 
    int char_width = font_size / 2;
    int str_len_px = strlen(msg) * char_width; 
    uint16_t startx = 0;
    if (str_len_px < lcddev.width)
    {
        startx = (lcddev.width - str_len_px) / 2;
    }
    lcd_show_string(startx, 245, lcddev.width, font_size, font_size, (char *)msg, YELLOW);
}

