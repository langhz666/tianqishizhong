/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-02-11 16:24:31
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-02-11 18:09:46
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

#define IMG_MEIHUA_W  180  
#define IMG_MEIHUA_H  180  

void welcome_page_display(void)
{
    /* 1. 设置背景色变量 */
    // lcd_show_string/chinese 依赖此变量作为文字背景
    g_back_color = BLACK;

    /* 2. 清屏 */
    // 对应 st7789_fill_color(..., mkcolor(0,0,0))
    lcd_clear(BLACK);

    /* 3. 显示梅花图片 */
    // 对应 st7789_draw_image(30, 10, &img_meihua);
    // 注意: 正点原子驱动需要传入具体的宽和高
    lcd_show_picture(30, 10, IMG_MEIHUA_W, IMG_MEIHUA_H, (const uint8_t *)&img_meihua);

    /* 4. 显示 "梅花嵌入式" (中文 32号字体) */
    lcd_show_chinese(40, 205, (uint8_t *)"梅花嵌入式", COLOR_PINK_MEIHUA, BLACK, &font32_maple_bold);

    /* 5. 显示 "天气时钟" (中文 32号字体) */
    lcd_show_chinese(56, 233, (uint8_t *)"天气时钟", COLOR_BLUE_TITLE, BLACK, &font32_maple_bold);

    /* 6. 显示 "loading..." (英文 24号字体) */
    // 对应 st7789_write_string(60, 285, ..., font24_maple_bold);
    // 注意: 原代码是自定义粗体，这里会变成标准 2412 字体
    // 参数: x, y, 区域宽, 区域高, 字体大小, 字符串, 颜色
    lcd_show_string(60, 285, 200, 24, 24, "loading...", WHITE);
}
