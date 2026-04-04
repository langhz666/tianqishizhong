#ifndef __TOUCH_H__
#define __TOUCH_H__

#include "stm32f4xx_hal.h"
#include "lcd.h"
#include "gt9xxx.h"
#include "ft5206.h"

#define T_PEN_GPIO_PORT                 GPIOB
#define T_PEN_GPIO_PIN                  GPIO_PIN_1
#define T_PEN_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define T_CS_GPIO_PORT                  GPIOC
#define T_CS_GPIO_PIN                   GPIO_PIN_13
#define T_CS_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define T_MISO_GPIO_PORT                GPIOB
#define T_MISO_GPIO_PIN                 GPIO_PIN_2
#define T_MISO_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define T_MOSI_GPIO_PORT                GPIOF
#define T_MOSI_GPIO_PIN                 GPIO_PIN_11
#define T_MOSI_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)

#define T_CLK_GPIO_PORT                 GPIOB
#define T_CLK_GPIO_PIN                  GPIO_PIN_0
#define T_CLK_GPIO_CLK_ENABLE()         do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define T_PEN           HAL_GPIO_ReadPin(T_PEN_GPIO_PORT, T_PEN_GPIO_PIN)
#define T_MISO          HAL_GPIO_ReadPin(T_MISO_GPIO_PORT, T_MISO_GPIO_PIN)

#define T_MOSI(x)     do{ x ? \
                          HAL_GPIO_WritePin(T_MOSI_GPIO_PORT, T_MOSI_GPIO_PIN, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(T_MOSI_GPIO_PORT, T_MOSI_GPIO_PIN, GPIO_PIN_RESET); \
                      }while(0)

#define T_CLK(x)      do{ x ? \
                          HAL_GPIO_WritePin(T_CLK_GPIO_PORT, T_CLK_GPIO_PIN, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(T_CLK_GPIO_PORT, T_CLK_GPIO_PIN, GPIO_PIN_RESET); \
                      }while(0)

#define T_CS(x)       do{ x ? \
                          HAL_GPIO_WritePin(T_CS_GPIO_PORT, T_CS_GPIO_PIN, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(T_CS_GPIO_PORT, T_CS_GPIO_PIN, GPIO_PIN_RESET); \
                      }while(0)

#define TP_PRES_DOWN    0x8000
#define TP_CATH_PRES    0x4000
#define CT_MAX_TOUCH    10

typedef struct
{
    uint8_t (*init)(void);
    uint8_t (*scan)(uint8_t);
    void (*adjust)(void);
    uint16_t x[CT_MAX_TOUCH];
    uint16_t y[CT_MAX_TOUCH];
    uint16_t sta;
    float xfac;
    float yfac;
    short xc;
    short yc;
    uint8_t touchtype;
} _m_tp_dev;

extern _m_tp_dev tp_dev;

uint8_t tp_init(void);
uint8_t tp_scan(uint8_t mode);
void tp_adjust(void);
void tp_save_adjust_data(void);
uint8_t tp_get_adjust_data(void);
void tp_draw_big_point(uint16_t x, uint16_t y, uint16_t color);

#endif
