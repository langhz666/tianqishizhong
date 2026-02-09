#include "led.h"


/**
 * @description: 设置LED状态
 * @param {uint8_t} led LED0 or LED1
 * @param {uin8_t} state    1: ON 0: OFF
 * @return {*}
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
 * @description: 开灯
 * @param {uint8_t} led
 * @return {*}
 */
void led_on(uint8_t led)
{
    led_set(led, 1);
}


/**
 * @description: 关灯
 * @param {uint8_t} led
 * @return {*}
 */
void led_off(uint8_t led)
{
    led_set(led, 0);
}


/**
 * @description: 关闭所有LED灯
 * @return {*}
 */
void led_all_off()
{
    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
}

/**
 * @description: 呼吸灯闪烁效果
 * @param {uint32_t} period
 * @param {uint32_t} duty
 * @return {*}
 */
void led_breath(uint32_t period, uint32_t duty)
{
    int a = 0;
    led_set(0, 1);
    led_set(1, 1);
    while (a++ < duty); //BREATH_TICK();
    led_set(0, 0);
    led_set(1, 0);
    while (a++ < period); //BREATH_TICK();
}

/**
 * @description: 切换LED状态
 * @param {uint8_t} led
 * @return {*}
 */
void led_toggle(uint8_t led)
{
    //led_set(led, !HAL_GPIO_ReadPin(led == 0 ? LED0_GPIO_Port : LED1_GPIO_Port, led == 0 ? LED0_Pin : LED1_Pin));
    if (led == 0)
    {
        HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
    }
    else if (led == 1)
    {
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
    }
}

