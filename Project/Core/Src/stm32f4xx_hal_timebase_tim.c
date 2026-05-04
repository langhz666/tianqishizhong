/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_hal_timebase_tim.c
  * @brief   基于硬件定时器的HAL时基配置
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef        htim1;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  配置TIM1作为HAL时基源
  *
  * 将TIM1配置为1ms时基，替代SysTick作为HAL的时间基准。
  * 此函数在HAL_Init()或HAL_RCC_ClockConfig()时自动调用。
  *
  * @param  TickPriority 定时器中断优先级
  * @retval HAL状态
  */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  RCC_ClkInitTypeDef    clkconfig;
  uint32_t              uwTimclock = 0U;

  uint32_t              uwPrescalerValue = 0U;
  uint32_t              pFLatency;

  HAL_StatusTypeDef     status;

  /* 使能TIM1时钟 */
  __HAL_RCC_TIM1_CLK_ENABLE();

  /* 获取时钟配置 */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

  /* 计算TIM1时钟频率（APB2定时器时钟 = 2 x APB2时钟） */
      uwTimclock = 2*HAL_RCC_GetPCLK2Freq();

  /* 计算预分频值，使TIM1计数器时钟为1MHz */
  uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000U) - 1U);

  /* 初始化TIM1 */
  htim1.Instance = TIM1;

  /* TIM1参数配置：
   * 周期 = [(TIM1CLK/1000) - 1]，产生1ms时基
   * 预分频 = (uwTimclock/1000000 - 1)，计数器时钟1MHz
   * 时钟分频 = 0
   * 计数方向 = 向上
   */
  htim1.Init.Period = (1000000U / 1000U) - 1U;
  htim1.Init.Prescaler = uwPrescalerValue;
  htim1.Init.ClockDivision = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  status = HAL_TIM_Base_Init(&htim1);
  if (status == HAL_OK)
  {
    /* 启动定时器中断模式 */
    status = HAL_TIM_Base_Start_IT(&htim1);
    if (status == HAL_OK)
    {
    /* 使能TIM1全局中断 */
        HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
      /* 配置SysTick中断优先级 */
      if (TickPriority < (1UL << __NVIC_PRIO_BITS))
      {
        /* 配置TIM中断优先级 */
        HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, TickPriority, 0U);
        uwTickPrio = TickPriority;
      }
      else
      {
        status = HAL_ERROR;
      }
    }
  }

  /* 返回函数状态 */
  return status;
}

/**
  * @brief  暂停Tick递增
  *
  * 禁用TIM1更新中断，暂停HAL时基计数。
  */
void HAL_SuspendTick(void)
{
  /* 禁用TIM1更新中断 */
  __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_UPDATE);
}

/**
  * @brief  恢复Tick递增
  *
  * 使能TIM1更新中断，恢复HAL时基计数。
  */
void HAL_ResumeTick(void)
{
  /* 使能TIM1更新中断 */
  __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE);
}

