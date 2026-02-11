#include <stdint.h>
#include <string.h>
#include "lcd.h"
#include "lcdfont.h"
#include "imag.h"
#include "font.h"

void error_page_display(const char *msg)
{
    /* 1. 设置背景色 */
    // 设置全局背景色变量，确保文字显示的背景是黑色的（因为 lcd_show_string 模式0依赖此变量）
    g_back_color = BLACK; 
    
    // 清屏为黑色
    // 对应原代码: st7789_fill_color(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1, color_bg);
    lcd_clear(BLACK);

    /* 2. 显示图片 */
    // 对应原代码: st7789_draw_image(40, 37, &img_error);
    // 注意：lcd_show_picture 需要具体的 宽(width) 和 高(height) 以及 数据指针
    // 你需要查看 imag.h 中 img_error 是怎么定义的。
    
    // 情况A：如果 img_error 是一个结构体 (包含 .width, .height, .data)
    lcd_show_picture(40, 37, img_error.width, img_error.height, img_error.data);
    
    // 情况B：如果 img_error 只是一个纯图像数组，你需要手动填入它的宽和高
    // 假设图片宽高是 100x100 (请根据实际图片修改!)
    //lcd_show_picture(40, 37, 100, 100, (const uint8_t *)&img_error); 
    
    /* 3. 计算居中显示的坐标 */
    // 原代码用 font20 (20号字体)，这里选用驱动支持的 24 号字体，显示更清晰
    uint8_t font_size = 24; 
    
    // 在标准 ASCII 字体中，字符宽度通常是高度的一半 (24 / 2 = 12像素)
    // 你的驱动 lcd_show_char 中也是这样计算的: (size / 2)
    int char_width = font_size / 2;
    int str_len_px = strlen(msg) * char_width; // 字符串总像素长度
    
    uint16_t startx = 0;
    if (str_len_px < lcddev.width)
    {
        startx = (lcddev.width - str_len_px) / 2;
    }

    /* 4. 显示字符串 */
    // 对应原代码: st7789_write_string(..., mkcolor(255, 255, 0), ...);
    // 255,255,0 对应 RGB565 的 YELLOW
    // 参数: x, y, width, height, size, string, color
    // width 设置为 lcddev.width 确保有足够空间显示
    lcd_show_string(startx, 245, lcddev.width, font_size, font_size, (char *)msg, YELLOW);
}

