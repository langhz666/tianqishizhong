/**
 * @file bsp_key.c
 * @brief 按键驱动模块
 *
 * 本文件实现了按键的读取、扫描和中断处理功能。
 * KEY0/KEY1/KEY2为低电平有效（按下接地），KEY_UP为高电平有效（按下接VCC）。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#include "bsp_key.h"

/**
 * @brief 读取指定按键的当前状态
 *
 * @param idx 按键编号（KEY_0、KEY_1、KEY_2、KEY_UP）
 * @return true 按键按下
 * @return false 按键未按下
 *
 * @note KEY0/KEY1/KEY2按下时读取到低电平（GPIO_PIN_RESET）
 * @note KEY_UP按下时读取到高电平（GPIO_PIN_SET）
 */
bool key_read(uint8_t idx)
{
    switch (idx)
    {
        case KEY_0:  return HAL_GPIO_ReadPin(KEY_0_GPIO_Port, KEY_0_Pin) == GPIO_PIN_RESET;
        case KEY_1:  return HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin) == GPIO_PIN_RESET;
        case KEY_2:  return HAL_GPIO_ReadPin(KEY_2_GPIO_Port, KEY_2_Pin) == GPIO_PIN_RESET;
        case KEY_UP: return HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == GPIO_PIN_SET;
        default:     return false;
    }
}

/**
 * @brief 按键扫描函数
 *
 * @param mode 0-不支持连按（按下一次只返回一次），1-支持连按
 * @retval 返回按下的键值，KEY_NONE_PRESS表示无按键
 */
uint8_t Key_Scan(uint8_t mode)
{
    static uint8_t key_up = 1; /* 按键松开标志 */
    if (mode) key_up = 1;      /* 支持连按模式 */

    /* 检测是否有按键按下 */
    if (key_up && (key_read(KEY_0) || key_read(KEY_1) || key_read(KEY_2) || key_read(KEY_UP)))
    {
        HAL_Delay(10); /* 消抖 */
        key_up = 0;    /* 标记按键已按下 */

        /* 再次确认具体是哪个键 */
        if (key_read(KEY_0))  return KEY0_PRESS;
        if (key_read(KEY_1))  return KEY1_PRESS;
        if (key_read(KEY_2))  return KEY2_PRESS;
        if (key_read(KEY_UP)) return KEYUP_PRESS;
    }
    /* 检测按键是否松开 */
    else if (!key_read(KEY_0) && !key_read(KEY_1) && !key_read(KEY_2) && !key_read(KEY_UP))
    {
        key_up = 1; /* 标记按键已松开 */
    }

    return KEY_NONE_PRESS;
}

/**
 * @brief GPIO外部中断回调函数
 *
 * 处理按键的外部中断事件，实现按键按下后的业务逻辑：
 * - KEY0：切换LED0状态
 * - KEY1：切换LED1状态
 * - KEY2：切换LED0和LED1状态，同时切换蜂鸣器
 * - KEY_UP：切换蜂鸣器状态
 *
 * @param GPIO_Pin 触发中断的GPIO引脚
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    HAL_Delay(20); /* 简单的阻塞式消抖 */

    switch(GPIO_Pin)
    {
        case KEY_0_Pin:
            if(HAL_GPIO_ReadPin(KEY_0_GPIO_Port, KEY_0_Pin) == GPIO_PIN_RESET)
            {
                led_toggle(0);
            }
            break;

        case KEY_1_Pin:
            if(HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin) == GPIO_PIN_RESET)
            {
                led_toggle(1);
            }
            break;

        case KEY_2_Pin:
            if(HAL_GPIO_ReadPin(KEY_2_GPIO_Port, KEY_2_Pin) == GPIO_PIN_RESET)
            {
                led_toggle(0);
                led_toggle(1);
                beep_toggle();
            }
            break;

        case KEY_UP_Pin:
            if(HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == GPIO_PIN_SET)
            {
                beep_toggle();
            }
            break;

        default:
            break;
    }
}
