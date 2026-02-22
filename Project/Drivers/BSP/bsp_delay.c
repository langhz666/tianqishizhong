#include "bsp_delay.h"

/**
 * @brief  初始化 DWT 计数器 (只需调用一次)
 */
void DWT_Delay_Init(void)
{
    /* 1. 关闭 TRC */
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
    /* 2. 开启 TRC */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* 3. 关闭计数器 */
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
    /* 4. 开启计数器 */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    /* 5. 计数器清零 */
    DWT->CYCCNT = 0;
}

/**
 * @brief  微秒级延时函数
 * @param  us: 要延时的微秒数
 */
void delay_us(uint32_t us)
{
    uint32_t startTick = DWT->CYCCNT;
    /* SystemCoreClock 在 F407 上通常是 168000000 */
    uint32_t delayTicks = us * (SystemCoreClock / 1000000);

    /* 循环等待，利用无符号数减法特性自动处理溢出 */
    while ((DWT->CYCCNT - startTick) < delayTicks);
}