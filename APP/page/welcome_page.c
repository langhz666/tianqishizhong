#include <stdint.h>
#include <string.h>
#include "lcd.h"
#include "lcdfont.h"
#include "font.h"
#include "imag.h"

// 1. 颜色定义 (RGB888 -> RGB565)
// 237, 128, 147 (粉色) -> 0xEC12
#define COLOR_PINK_MEIHUA  0xEC12 
// 86, 165, 255 (浅蓝) -> 0x553F
#define COLOR_BLUE_TITLE   0x553F

// 如果 lcd.h 没定义 FONT_SIZE 枚举，这里补齐以防报错
#ifndef FONT_SIZE_32
#define FONT_SIZE_32 32
#endif

// 2. 图片尺寸定义 (请务必修改为 img_meihua 的实际宽高!)
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
    // 对应 st7789_write_string(40, 205, ..., font32_maple_bold);
    // 颜色: 粉色, 背景: 黑色
    // 前提: cn_32x32 数组里必须有这些汉字的取模数据
    lcd_show_chinese(40, 205, (uint8_t *)"梅花嵌入式", COLOR_PINK_MEIHUA, BLACK, FONT_SIZE_32);

    /* 5. 显示 "天气时钟" (中文 32号字体) */
    // 对应 st7789_write_string(56, 233, ..., font32_maple_bold);
    // 颜色: 浅蓝, 背景: 黑色
    lcd_show_chinese(56, 233, (uint8_t *)"天气时钟", COLOR_BLUE_TITLE, BLACK, FONT_SIZE_32);

    /* 6. 显示 "loading..." (英文 24号字体) */
    // 对应 st7789_write_string(60, 285, ..., font24_maple_bold);
    // 注意: 原代码是自定义粗体，这里会变成标准 2412 字体
    // 参数: x, y, 区域宽, 区域高, 字体大小, 字符串, 颜色
    lcd_show_string(60, 285, 200, 24, 24, "loading...", WHITE);
}
