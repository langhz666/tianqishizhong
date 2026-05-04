/**
 * @file led.h
 * @brief LED驱动模块头文件
 *
 * 本头文件定义了LED控制的函数接口。
 * 支持2个LED：LED0（PF9）和LED1（PF10），低电平点亮。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __LED_H__
#define __LED_H__

#include "main.h"

/**
 * @brief 设置LED状态
 *
 * @param led LED编号（0=LED0, 1=LED1）
 * @param state 1=点亮, 0=熄灭
 */
void led_set(uint8_t led, uint8_t state);

/**
 * @brief 点亮指定LED
 *
 * @param led LED编号（0=LED0, 1=LED1）
 */
void led_on(uint8_t led);

/**
 * @brief 熄灭指定LED
 *
 * @param led LED编号（0=LED0, 1=LED1）
 */
void led_off(uint8_t led);

/**
 * @brief 关闭所有LED灯
 */
void led_all_off(void);

/**
 * @brief 呼吸灯闪烁效果
 *
 * @param period 周期
 * @param duty 占空比
 */
void led_breath(uint32_t period, uint32_t duty);

/**
 * @brief 切换LED状态
 *
 * @param led LED编号（0=LED0, 1=LED1）
 */
void led_toggle(uint8_t led);

#endif /* __LED_H__ */
