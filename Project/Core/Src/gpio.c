/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   GPIO引脚配置与初始化
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
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* GPIO 引脚配置                                                              */
/*                                                                            */
/* 引脚分配表：                                                               */
/*   PE2/PE3/PE4  - KEY0/KEY1/KEY2 (输入，下降沿中断，内部上拉)               */
/*   PA0          - KEY_UP (输入，上升沿中断，内部下拉)                        */
/*   PF6          - DHT11_DQ (开漏输出，外部上拉)                             */
/*   PF8          - BEEP (推挽输出)                                           */
/*   PF9          - LED0 (推挽输出，低电平点亮)                                */
/*   PF10         - LED1 (推挽输出，低电平点亮)                                */
/*   PC13         - T_CS (触摸片选，推挽输出)                                  */
/*   PB0          - T_CLK (触摸时钟，推挽输出)                                */
/*   PB1          - T_PEN (触摸中断，输入，内部上拉)                           */
/*   PB2          - T_MISO (触摸数据输入)                                     */
/*   PF11         - T_MOSI (触摸数据输出，推挽)                                */
/*   PB15         - LCD_BL (LCD背光，推挽输出，高电平点亮)                     */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** @brief 初始化所有配置的GPIO引脚
  *
  * 使能GPIO端口时钟，配置各引脚的工作模式、上下拉、速度等参数。
  * 此函数由main()在系统启动时调用。
  */
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 使能GPIO端口时钟 */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /* 设置GPIO引脚默认输出电平 */
  HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, GPIO_PIN_SET);           /* 触摸片选默认高（未选中） */
  HAL_GPIO_WritePin(GPIOF, DHT11_DQ_Pin|BEEP_Pin|T_MOSI_Pin, GPIO_PIN_RESET);  /* DHT11/蜂鸣器/触摸MOSI默认低 */
  HAL_GPIO_WritePin(GPIOF, LED0_Pin|LED1_Pin, GPIO_PIN_SET);           /* LED默认高（熄灭，低电平点亮） */
  HAL_GPIO_WritePin(T_CLK_GPIO_Port, T_CLK_Pin, GPIO_PIN_RESET);      /* 触摸时钟默认低 */
  HAL_GPIO_WritePin(LCD_BL_GPIO_Port, LCD_BL_Pin, GPIO_PIN_SET);       /* LCD背光默认高（点亮） */

  /* 配置按键引脚：KEY0/KEY1/KEY2 (PE2/PE3/PE4) - 下降沿触发中断，内部上拉 */
  GPIO_InitStruct.Pin = KEY_2_Pin|KEY_1_Pin|KEY_0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* 配置触摸片选引脚：T_CS (PC13) - 推挽输出，无上下拉，低速 */
  GPIO_InitStruct.Pin = T_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(T_CS_GPIO_Port, &GPIO_InitStruct);

  /* 配置DHT11数据引脚：DHT11_DQ (PF6) - 开漏输出，内部上拉，高速 */
  GPIO_InitStruct.Pin = DHT11_DQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(DHT11_DQ_GPIO_Port, &GPIO_InitStruct);

  /* 配置推挽输出引脚：BEEP(PF8)/LED0(PF9)/LED1(PF10)/T_MOSI(PF11) */
  GPIO_InitStruct.Pin = BEEP_Pin|LED0_Pin|LED1_Pin|T_MOSI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /* 配置KEY_UP引脚：PA0 - 上升沿触发中断，内部下拉 */
  GPIO_InitStruct.Pin = KEY_UP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(KEY_UP_GPIO_Port, &GPIO_InitStruct);

  /* 配置触摸时钟引脚：T_CLK (PB0) - 推挽输出，高速 */
  GPIO_InitStruct.Pin = T_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(T_CLK_GPIO_Port, &GPIO_InitStruct);

  /* 配置触摸中断引脚：T_PEN (PB1) - 输入模式，内部上拉 */
  GPIO_InitStruct.Pin = T_PEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(T_PEN_GPIO_Port, &GPIO_InitStruct);

  /* 配置触摸数据输入引脚：T_MISO (PB2) - 输入模式，无上下拉 */
  GPIO_InitStruct.Pin = T_MISO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(T_MISO_GPIO_Port, &GPIO_InitStruct);

  /* 配置LCD背光引脚：LCD_BL (PB15) - 推挽输出，内部上拉，高速 */
  GPIO_InitStruct.Pin = LCD_BL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LCD_BL_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
