#include "lv_port_disp.h"
#include <stdbool.h>
#include "lcd.h"

#define MY_DISP_HOR_RES    240
#define MY_DISP_VER_RES    320

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

void lv_port_disp_init(void)
{
    lcd_clear(BLACK);

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[MY_DISP_HOR_RES * 40];
    static lv_color_t buf2[MY_DISP_HOR_RES * 40];

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, MY_DISP_HOR_RES * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;

    lv_disp_drv_register(&disp_drv);
}

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    uint16_t x1 = area->x1;
    uint16_t y1 = area->y1;
    uint16_t x2 = area->x2;
    uint16_t y2 = area->y2;

    lcd_set_window(x1, y1, x2 - x1 + 1, y2 - y1 + 1);

    uint32_t len = (x2 - x1 + 1) * (y2 - y1 + 1);

    LCD->LCD_REG = lcddev.wramcmd;

    for (uint32_t i = 0; i < len; i++) {
        LCD->LCD_RAM = color_p[i].full;
    }

    lv_disp_flush_ready(disp_drv);
}
