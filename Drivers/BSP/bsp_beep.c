#include "bsp_beep.h"



/**
 * @description: 打开蜂鸣器

 * @return {*}
 */
void beep_on(void)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
}

/**
 * @description: 关闭蜂鸣器
 * @return {*}
 */
void beep_off(void)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
}

/**
 * @description: 蜂鸣器状态切换函数
 * @return {*}
 */
void beep_toggle(void)
{
    HAL_GPIO_TogglePin(BEEP_GPIO_Port, BEEP_Pin);
}
