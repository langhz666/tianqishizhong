/**
 * @file bsp_beep.c
 * @brief 蜂鸣器驱动模块
 *
 * 本文件实现了蜂鸣器的基本控制功能：开启、关闭和状态切换。
 * 蜂鸣器连接到PF8引脚，高电平有效。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#include "bsp_beep.h"

/**
 * @brief 打开蜂鸣器
 */
void beep_on(void)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
}

/**
 * @brief 关闭蜂鸣器
 */
void beep_off(void)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 切换蜂鸣器状态
 */
void beep_toggle(void)
{
    HAL_GPIO_TogglePin(BEEP_GPIO_Port, BEEP_Pin);
}
