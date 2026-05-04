/**
 * @file bsp_key.h
 * @brief 按键驱动模块头文件
 *
 * 本头文件定义了按键驱动的引脚编号、键值和函数接口。
 * 支持4个按键：KEY0、KEY1、KEY2（低电平有效）和KEY_UP（高电平有效）。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __BSP_KEY_H__
#define __BSP_KEY_H__

#include "main.h"
#include "stdbool.h"
#include "led.h"
#include "bsp_beep.h"

/*============================================================================*/
/*                             按键引脚编号                                   */
/*============================================================================*/

#define KEY_0   0       /**< 按键0引脚编号 */
#define KEY_1   1       /**< 按键1引脚编号 */
#define KEY_2   2       /**< 按键2引脚编号 */
#define KEY_UP  3       /**< 按键UP引脚编号 */

/*============================================================================*/
/*                             按键键值定义                                   */
/*============================================================================*/

#define KEY_NONE_PRESS  0   /**< 无按键按下 */
#define KEY0_PRESS      1   /**< 按键0按下 */
#define KEY1_PRESS      2   /**< 按键1按下 */
#define KEY2_PRESS      3   /**< 按键2按下 */
#define KEYUP_PRESS     4   /**< 按键UP按下 */

/*============================================================================*/
/*                             驱动接口声明                                   */
/*============================================================================*/

/**
 * @brief 读取指定按键的当前状态
 *
 * @param idx 按键编号（KEY_0、KEY_1、KEY_2、KEY_UP）
 * @return true 按键按下
 * @return false 按键未按下
 */
bool key_read(uint8_t idx);

/**
 * @brief 按键扫描函数
 *
 * @param mode 0-不支持连按（按下一次只返回一次），1-支持连按
 * @retval 返回按下的键值，KEY_NONE_PRESS表示无按键
 */
uint8_t Key_Scan(uint8_t mode);

#endif /* __BSP_KEY_H__ */
