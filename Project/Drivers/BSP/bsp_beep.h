/**
 * @file bsp_beep.h
 * @brief 蜂鸣器驱动模块头文件
 *
 * 本头文件定义了蜂鸣器控制的函数接口。
 * 蜂鸣器通过GPIO引脚控制，高电平响，低电平灭。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __BSP_BEEP_H__
#define __BSP_BEEP_H__

#include "main.h"

/**
 * @brief 打开蜂鸣器
 */
void beep_on(void);

/**
 * @brief 关闭蜂鸣器
 */
void beep_off(void);

/**
 * @brief 切换蜂鸣器状态
 */
void beep_toggle(void);

#endif /* __BSP_BEEP_H__ */
