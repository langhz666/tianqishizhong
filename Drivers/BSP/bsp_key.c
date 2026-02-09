#include "bsp_key.h"


/**
 * @description: 读取按键状态
 * @param {uint8_t} idx
 * @return {*}
 */
bool key_read(uint8_t idx)
{
    switch (idx)
    {
        // 这里的 GPIO_PIN_SET 代表读取到高电平 (1)
        // KEY0, KEY1, KEY2 是按键，按下时通常会连接到地，所以读取到的是低电平 (0)，因此比较时使用 GPIO_PIN_RESET
        // KEY_UP 是按键，按下时通常会连接到地，所以读取到是高电平 (1)，因此比较时使用 GPIO_PIN_SET
        case KEY_0:  return HAL_GPIO_ReadPin(KEY_0_GPIO_Port, KEY_0_Pin) == GPIO_PIN_RESET;
        case KEY_1:  return HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin) == GPIO_PIN_RESET;
        case KEY_2:  return HAL_GPIO_ReadPin(KEY_2_GPIO_Port, KEY_2_Pin) == GPIO_PIN_RESET;
        case KEY_UP:  return HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == GPIO_PIN_SET;
        default: return false;
    }
}

/**
 * @brief  按键扫描函数
 * @param  mode: 0-不支持连按(按下一次只返回一次), 1-支持连按
 * @retval 返回按下的键值，0表示无按键
 */
uint8_t Key_Scan(uint8_t mode)
{
    static uint8_t key_up = 1; // 按键松开标志
    if (mode) key_up = 1;      // 支持连按模式

    // 1. 检测是否有按键按下 (注意 KEY_UP 是高电平有效，其他是低电平，这里假设 key_read 已经封装好逻辑)
    if (key_up && (key_read(KEY_0) || key_read(KEY_1) || key_read(KEY_2) || key_read(KEY_UP)))
    {
        HAL_Delay(10); // 消抖
        key_up = 0;    // 标记按键已按下

        // 2. 再次确认具体是哪个键
        if (key_read(KEY_0))  return KEY0_PRESS;
        if (key_read(KEY_1))  return KEY1_PRESS;
        if (key_read(KEY_2))  return KEY2_PRESS;
        if (key_read(KEY_UP)) return KEYUP_PRESS;
    }
    // 3. 检测按键是否松开
    else if (!key_read(KEY_0) && !key_read(KEY_1) && !key_read(KEY_2) && !key_read(KEY_UP))
    {
        key_up = 1; // 标记按键已松开
    }
    
    return KEY_NONE_PRESS;
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

    HAL_Delay(20); // 简单的阻塞式消抖，实际项目中建议使用非阻塞方式
    // 3. 处理业务逻辑
    switch(GPIO_Pin)
    {
        case KEY_0_Pin: 
            if(HAL_GPIO_ReadPin(KEY_0_GPIO_Port, KEY_0_Pin) == GPIO_PIN_RESET) 
            {
                led_toggle(0); // 切换 LED0 状态
            }
            break;

        case KEY_1_Pin:
            if(HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin) == GPIO_PIN_RESET) 
            {
                led_toggle(1); // 切换 LED1 状态
            }
            break;

        case KEY_2_Pin:
             if(HAL_GPIO_ReadPin(KEY_2_GPIO_Port, KEY_2_Pin) == GPIO_PIN_RESET) 
            {
                led_toggle(0);
                led_toggle(1);
                beep_toggle(); // 切换蜂鸣器状态
            }
            break;

        case KEY_UP_Pin:
             if(HAL_GPIO_ReadPin(KEY_UP_GPIO_Port, KEY_UP_Pin) == GPIO_PIN_SET) 
            {
                beep_toggle(); // 切换蜂鸣器状态
            }
            break;
            
        default:
            break;
    }
}