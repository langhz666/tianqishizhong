/**
 * @file led.c
 * @brief LED驱动模块
 *
 * 本文件实现了LED的基本控制功能：设置、开关、切换和呼吸灯效果。
 * LED0连接到PF9，LED1连接到PF10，均为低电平点亮。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#include "led.h"

/**
 * @brief 设置LED状态
 *
 * @param led LED编号（0=LED0, 1=LED1）
 * @param state 1=点亮, 0=熄灭
 *
 * @note LED低电平点亮，因此state=1时输出GPIO_PIN_RESET
 */
void led_set(uint8_t led, uint8_t state)
{
    if (led == 0)
    {
        HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, state ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
    else if (led == 1)
    {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, state ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
}

/**
 * @brief 点亮指定LED
 *
 * @param led LED编号（0=LED0, 1=LED1）
 */
void led_on(uint8_t led)
{
    led_set(led, 1);
}

/**
 * @brief 熄灭指定LED
 *
 * @param led LED编号（0=LED0, 1=LED1）
 */
void led_off(uint8_t led)
{
    led_set(led, 0);
}

/**
 * @brief 关闭所有LED灯
 */
void led_all_off(void)
{
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
}

/**
 * @brief 软件模拟呼吸灯效果（忙等实现）
 *
 * 通过忙等循环控制LED亮灭时间比例，非标准PWM实现。
 * LED点亮持续 duty 次循环，熄灭持续 (period - duty) 次循环。
 *
 * @param period 总周期（忙等循环次数）
 * @param duty   LED点亮时间（忙等循环次数，应小于period）
 */
void led_breath(uint32_t period, uint32_t duty)
{
    int a = 0;
    led_set(0, 1);
    led_set(1, 1);
    while (a++ < duty);
    led_set(0, 0);
    led_set(1, 0);
    while (a++ < period);
}

/**
 * @brief 切换LED状态
 *
 * @param led LED编号（0=LED0, 1=LED1）
 */
void led_toggle(uint8_t led)
{
    if (led == 0)
    {
        HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
    }
    else if (led == 1)
    {
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    }
}
