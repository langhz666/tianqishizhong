/**
 * @file bsp_delay.c
 * @brief DWT微秒延时驱动
 *
 * 使用ARM Cortex-M内核的DWT（Data Watchpoint and Trace）周期计数器
 * 实现高精度微秒级延时。DWT计数器以CPU主频运行（168MHz），
 * 因此延时精度可达约6ns级别。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#include "bsp_delay.h"

/**
 * @brief 初始化DWT计数器（只需调用一次）
 *
 * 启用DWT跟踪功能并启动周期计数器
 */
void DWT_Delay_Init(void)
{
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;  /* 关闭跟踪 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   /* 开启跟踪 */
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;             /* 关闭计数器 */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;              /* 开启计数器 */
    DWT->CYCCNT = 0;                                   /* 计数器清零 */
}

/**
 * @brief 微秒级延时函数
 *
 * @param us 要延时的微秒数
 *
 * @note SystemCoreClock在F407上为168000000（168MHz）
 */
void delay_us(uint32_t us)
{
    uint32_t startTick = DWT->CYCCNT;
    uint32_t delayTicks = us * (SystemCoreClock / 1000000);

    /* 循环等待，利用无符号数减法特性自动处理溢出 */
    while ((DWT->CYCCNT - startTick) < delayTicks);
}
