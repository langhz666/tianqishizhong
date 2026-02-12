/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-02-11 16:24:31
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-02-12 10:46:34
 * @FilePath: \luoji\APP\page\welcome_page.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdint.h>
#include <string.h>
#include "lcd.h"
#include "lcdfont.h"
#include "font.h"
#include "imag.h"

#define COLOR_PINK_MEIHUA  0xEC12 
#define COLOR_BLUE_TITLE   0x553F


void welcome_page_display(void)
{
    g_back_color = BLACK;
    lcd_fill(0, 0, lcddev.width - 1, lcddev.height - 1, BLACK);
    lcd_show_picture(30, 10, img_meihua.width, img_meihua.height, img_meihua.data);
    lcd_show_chinese(40, 205, (uint8_t *)"梅花嵌入式", COLOR_PINK_MEIHUA, BLACK, &font32_maple_bold);
    lcd_show_chinese(56, 233, (uint8_t *)"天气时钟", COLOR_BLUE_TITLE, BLACK, &font32_maple_bold);
    lcd_show_string(60, 285, 200, 24, 24, "loading...", WHITE);
}
