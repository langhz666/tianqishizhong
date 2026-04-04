#include "stdio.h"
#include "stdlib.h"
#include "lcd.h"
#include "touch.h"
#include "bsp_delay.h"

#define XPT2046_CMD_X    0xD0
#define XPT2046_CMD_Y    0x90

#define TP_X_MIN         100
#define TP_X_MAX         3900
#define TP_Y_MIN         100
#define TP_Y_MAX         3900

static uint8_t tp_spi_wr(uint8_t data);
static uint16_t tp_read_ad(uint8_t cmd);
static uint16_t tp_read_xy(uint8_t cmd);
static void tp_read_xy2(uint16_t *x, uint16_t *y);
static uint8_t tp_scan_internal(uint8_t mode);

_m_tp_dev tp_dev =
{
    .init = tp_init,
    .scan = tp_scan_internal,
    .adjust = tp_adjust,
    .x = {0},
    .y = {0},
    .sta = 0,
    .xfac = 0.1f,
    .yfac = 0.1f,
    .xc = 0,
    .yc = 0,
    .touchtype = 0,
};

static uint8_t tp_spi_wr(uint8_t data)
{
    uint8_t i;
    uint8_t ret = 0;

    for (i = 0; i < 8; i++)
    {
        T_CLK(0);
        
        if (data & 0x80)
        {
            T_MOSI(1);
        }
        else
        {
            T_MOSI(0);
        }
        
        data <<= 1;
        
        T_CLK(1);
        
        ret <<= 1;
        if (T_MISO)
        {
            ret |= 0x01;
        }
    }
    
    T_CLK(0);
    
    return ret;
}

static uint16_t tp_read_ad(uint8_t cmd)
{
    uint16_t value = 0;

    T_CS(0);
    
    tp_spi_wr(cmd);
    
    delay_us(10);
    
    value = tp_spi_wr(0x00);
    value <<= 8;
    value |= tp_spi_wr(0x00);
    
    value >>= 3;
    
    T_CS(1);
    
    return value;
}

static uint16_t tp_read_xy(uint8_t cmd)
{
    uint16_t i;
    uint16_t value;
    uint16_t sum = 0;
    uint16_t buf[10];
    uint16_t temp;

    for (i = 0; i < 10; i++)
    {
        buf[i] = tp_read_ad(cmd);
    }
    
    for (i = 0; i < 9; i++)
    {
        for (uint8_t j = i + 1; j < 10; j++)
        {
            if (buf[i] > buf[j])
            {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }
    
    for (i = 1; i < 9; i++)
    {
        sum += buf[i];
    }
    
    value = sum >> 3;
    
    return value;
}

static void tp_read_xy2(uint16_t *x, uint16_t *y)
{
    uint16_t x1, y1;
    uint16_t x2, y2;
    
    x1 = tp_read_xy(XPT2046_CMD_X);
    y1 = tp_read_xy(XPT2046_CMD_Y);
    
    x2 = tp_read_xy(XPT2046_CMD_X);
    y2 = tp_read_xy(XPT2046_CMD_Y);
    
    if (x1 > x2)
    {
        *x = (x1 - x2) > 50 ? 0 : ((x1 + x2) >> 1);
    }
    else
    {
        *x = (x2 - x1) > 50 ? 0 : ((x1 + x2) >> 1);
    }
    
    if (y1 > y2)
    {
        *y = (y1 - y2) > 50 ? 0 : ((y1 + y2) >> 1);
    }
    else
    {
        *y = (y2 - y1) > 50 ? 0 : ((y1 + y2) >> 1);
    }
}

uint8_t tp_scan(uint8_t mode)
{
    (void)mode;
    return tp_dev.scan(0);
}

static void tp_draw_cross(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_line(x - 12, y, x + 12, y, color);
    lcd_draw_line(x, y - 12, x, y + 12, color);
    lcd_draw_point(x, y, color);
    lcd_draw_point(x + 1, y, color);
    lcd_draw_point(x, y + 1, color);
    lcd_draw_point(x - 1, y, color);
    lcd_draw_point(x, y - 1, color);
}

static uint8_t tp_wait_for_press(uint16_t *x, uint16_t *y)
{
    uint16_t adx, ady;
    uint32_t timeout = 0;
    
    while (timeout < 50000)
    {
        if (T_PEN == 0)
        {
            tp_read_xy2(&adx, &ady);
            *x = adx;
            *y = ady;
            
            HAL_Delay(20);
            
            while (T_PEN == 0)
            {
                HAL_Delay(10);
            }
            
            return 1;
        }
        
        HAL_Delay(1);
        timeout++;
    }
    
    return 0;
}

void tp_adjust(void)
{
    uint16_t pos_temp[4];
    uint16_t adx, ady;
    uint16_t px, py;
    float xfac, yfac;
    short xc, yc;
    
    lcd_clear(WHITE);
    
    lcd_show_string(lcddev.width / 2 - 60, 10, 200, 24, 24, (char *)"Touch Adjust", BLUE);
    lcd_show_string(lcddev.width / 2 - 80, 40, 200, 16, 16, (char *)"Please touch the cross", BLUE);
    
    px = 20;
    py = 20;
    tp_draw_cross(px, py, RED);
    
    while (tp_wait_for_press(&adx, &ady) == 0)
    {
        HAL_Delay(10);
    }
    pos_temp[0] = adx;
    pos_temp[1] = ady;
    lcd_fill(px - 15, py - 15, px + 15, py + 15, WHITE);
    HAL_Delay(500);
    
    px = lcddev.width - 20;
    py = 20;
    tp_draw_cross(px, py, RED);
    
    while (tp_wait_for_press(&adx, &ady) == 0)
    {
        HAL_Delay(10);
    }
    pos_temp[2] = adx;
    pos_temp[3] = ady;
    lcd_fill(px - 15, py - 15, px + 15, py + 15, WHITE);
    HAL_Delay(500);
    
    px = 20;
    py = lcddev.height - 20;
    tp_draw_cross(px, py, RED);
    
    while (tp_wait_for_press(&adx, &ady) == 0)
    {
        HAL_Delay(10);
    }
    HAL_Delay(500);
    lcd_fill(px - 15, py - 15, px + 15, py + 15, WHITE);
    
    px = lcddev.width - 20;
    py = lcddev.height - 20;
    tp_draw_cross(px, py, RED);
    
    while (tp_wait_for_press(&adx, &ady) == 0)
    {
        HAL_Delay(10);
    }
    HAL_Delay(500);
    lcd_fill(px - 15, py - 15, px + 15, py + 15, WHITE);
    
    xfac = (float)(lcddev.width - 40) / (pos_temp[2] - pos_temp[0]);
    yfac = (float)(lcddev.height - 40) / (pos_temp[3] - pos_temp[1]);
    
    xc = pos_temp[0];
    yc = pos_temp[1];
    
    tp_dev.xfac = xfac;
    tp_dev.yfac = yfac;
    tp_dev.xc = xc;
    tp_dev.yc = yc;
    
    printf("Adjust done: xfac=%.4f, yfac=%.4f, xc=%d, yc=%d\r\n", 
           xfac, yfac, xc, yc);
    
    lcd_clear(WHITE);
    lcd_show_string(lcddev.width / 2 - 60, lcddev.height / 2 - 12, 200, 24, 24, (char *)"Adjust OK!", BLUE);
    HAL_Delay(1000);
    lcd_clear(WHITE);
}

void tp_save_adjust_data(void)
{
}

uint8_t tp_get_adjust_data(void)
{
    return 0;
}

void tp_draw_big_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_point(x, y, color);
    lcd_draw_point(x + 1, y, color);
    lcd_draw_point(x, y + 1, color);
    lcd_draw_point(x + 1, y + 1, color);
}

uint8_t tp_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    
    tp_dev.touchtype = 0;
    tp_dev.touchtype |= lcddev.dir & 0X01;

    if (gt9xxx_init() == 0)
    {
        tp_dev.scan = gt9xxx_scan;
        tp_dev.touchtype |= 0X80;
        printf("CTP: GT911 init OK\r\n");
        return 0;
    }

    T_PEN_GPIO_CLK_ENABLE();
    T_CS_GPIO_CLK_ENABLE();
    T_MISO_GPIO_CLK_ENABLE();
    T_MOSI_GPIO_CLK_ENABLE();
    T_CLK_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin = T_PEN_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(T_PEN_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = T_MISO_GPIO_PIN;
    HAL_GPIO_Init(T_MISO_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = T_MOSI_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(T_MOSI_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = T_CLK_GPIO_PIN;
    HAL_GPIO_Init(T_CLK_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = T_CS_GPIO_PIN;
    HAL_GPIO_Init(T_CS_GPIO_PORT, &gpio_init_struct);

    T_CS(1);
    T_CLK(0);
    T_MOSI(1);

    tp_dev.xfac = (float)lcddev.width / (TP_X_MAX - TP_X_MIN);
    tp_dev.yfac = (float)lcddev.height / (TP_Y_MAX - TP_Y_MIN);
    tp_dev.xc = TP_X_MIN;
    tp_dev.yc = TP_Y_MIN;

    printf("RTP: XPT2046 init OK\r\n");
    printf("LCD: %dx%d\r\n", lcddev.width, lcddev.height);

    return 1;
}

static uint8_t tp_scan_internal(uint8_t mode)
{
    (void)mode;
    
    uint16_t adx, ady;
    static uint8_t press_flag = 0;
    
    if (T_PEN == 0)
    {
        tp_read_xy2(&adx, &ady);
        
        if (adx >= TP_X_MIN && adx <= TP_X_MAX && 
            ady >= TP_Y_MIN && ady <= TP_Y_MAX)
        {
            if (tp_dev.touchtype & 0X01)
            {
                tp_dev.x[0] = lcddev.width - (uint16_t)((adx - tp_dev.xc) * tp_dev.xfac);
                tp_dev.y[0] = (uint16_t)((ady - tp_dev.yc) * tp_dev.yfac);
            }
            else
            {
                tp_dev.x[0] = (uint16_t)((adx - tp_dev.xc) * tp_dev.xfac);
                tp_dev.y[0] = (uint16_t)((ady - tp_dev.yc) * tp_dev.yfac);
            }
            
            if (tp_dev.x[0] >= lcddev.width) tp_dev.x[0] = lcddev.width - 1;
            if (tp_dev.y[0] >= lcddev.height) tp_dev.y[0] = lcddev.height - 1;
            
            if (press_flag == 0)
            {
                tp_dev.sta = TP_PRES_DOWN;
                press_flag = 1;
            }
            else
            {
                tp_dev.sta = TP_PRES_DOWN | TP_CATH_PRES;
            }
            
            return 1;
        }
    }
    
    if (press_flag)
    {
        press_flag = 0;
        tp_dev.sta = 0;
    }
    
    return 0;
}
