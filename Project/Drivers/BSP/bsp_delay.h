/**
 * @file bsp_delay.h
 * @brief DWT微秒延时驱动头文件
 *
 * 使用ARM Cortex-M内核的DWT（Data Watchpoint and Trace）计数器
 * 实现高精度微秒级延时，比HAL_Delay()更精确。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __BSP_DELAY_H__
#define __BSP_DELAY_H__

#include "stm32f4xx_hal.h"

/**
 * @brief 初始化DWT计数器
 *
 * 启用DWT周期计数器，用于微秒级延时。只需调用一次。
 */
void DWT_Delay_Init(void);

/**
 * @brief 微秒级延时函数
 *
 * @param us 要延时的微秒数
 */
void delay_us(uint32_t us);

#endif /* __BSP_DELAY_H__ */
