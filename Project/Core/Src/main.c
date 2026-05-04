/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 天气时钟系统主程序入口
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
#include "main.h"
#include "cmsis_os.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led.h"
#include "bsp_usart.h"
#include "bsp_key.h"
#include "bsp_dht11.h"
#include "bsp_delay.h"
#include "lcd.h"
#include "weather.h"
#include "bsp_espat.h"
#include "page.h"
#include "app.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief FreeRTOS断言失败回调
 *
 * 当FreeRTOS内部断言检查失败时调用，禁用中断并进入死循环
 *
 * @param file 断言失败的文件名
 * @param line 断言失败的行号
 */
void vAssertCalled(const char *file, int line)
{
    (void)file;
    (void)line;
    taskDISABLE_INTERRUPTS();
    for (;;);
}

/**
 * @brief FreeRTOS栈溢出钩子函数
 *
 * 当检测到任务栈溢出时调用，禁用中断并进入死循环
 *
 * @param xTask 发生溢出的任务句柄
 * @param pcTaskName 发生溢出的任务名称
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;);
}

/**
 * @brief FreeRTOS内存分配失败钩子函数
 *
 * 当pvPortMalloc()分配内存失败时调用，禁用中断并进入死循环
 */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;);
}
/* USER CODE END 0 */

/**
  * @brief  应用程序入口函数
  *
  * 系统启动流程：
  * 1. HAL库初始化（Flash预取、SysTick配置）
  * 2. 系统时钟配置（HSE 8MHz -> PLL -> 168MHz）
  * 3. 外设初始化（GPIO、UART、FSMC、RTC）
  * 4. 驱动初始化（DWT延时、DHT11、LCD、LVGL）
  * 5. 显示欢迎页面
  * 6. 启动FreeRTOS调度器
  *
  * @retval int 正常情况下不会返回
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* 复位所有外设，初始化Flash接口和SysTick定时器 */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* 配置系统时钟 */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* 初始化所有配置的外设 */
  MX_GPIO_Init();
  MX_USART1_UART_Init();    /* USART1: 调试串口 (PA9/PA10, 115200) */
  MX_FSMC_Init();            /* FSMC: LCD数据总线 (Bank1 NE4, 16bit) */
  MX_USART2_UART_Init();    /* USART2: ESP模块通信 (PA2/PA3, 115200) */
  MX_RTC_Init();             /* RTC: 实时时钟 (LSE 32.768kHz) */

  /* USER CODE BEGIN 2 */
  printf("[MAIN] Hardware init complete\n");
  printf("[MAIN] SystemCoreClock = %lu\n", SystemCoreClock);

  DWT_Delay_Init();
  printf("[MAIN] DWT init done\n");

  dht11_init();
  printf("[MAIN] DHT11 init done\n");

  lcd_init();
  printf("[MAIN] LCD init done\n");

  lv_init();
  lv_port_disp_init();
  lv_port_indev_init();
  printf("[MAIN] LVGL init done\n");

  welcome_page_display();
  printf("[MAIN] Welcome page displayed\n");

  /* USER CODE END 2 */

  /* 初始化FreeRTOS调度器 */
  osKernelInitialize();
  MX_FREERTOS_Init();

  /* 启动调度器（此函数不会返回） */
  osKernelStart();

  /* 正常情况下不会执行到这里，控制权已交给调度器 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief 系统时钟配置
  *
  * 时钟树配置：
  * - HSE (8MHz外部晶振) 作为PLL输入
  * - PLLM=4, PLLN=168, PLLP=2 -> SYSCLK=168MHz
  * - AHB分频=1 -> HCLK=168MHz
  * - APB1分频=4 -> PCLK1=42MHz (Timer=84MHz)
  * - APB2分频=2 -> PCLK2=84MHz (Timer=168MHz)
  * - LSE (32.768kHz外部晶振) 用于RTC
  *
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** 配置主内部稳压器输出电压 */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** 初始化RCC振荡器 */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** 初始化CPU、AHB和APB总线时钟 */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  定时器周期溢出回调函数（非阻塞模式）
  *
  * 当TIM1中断发生时，在HAL_TIM_IRQHandler()中直接调用此函数
  * 递增全局变量"uwTick"作为应用时间基准
  *
  * @param  htim : TIM句柄
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  错误处理函数
  *
  * 当发生不可恢复的错误时调用，禁用中断并进入死循环
  *
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  断言失败报告函数
  *
  * @param  file: 源文件名指针
  * @param  line: 断言失败的行号
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* 用户可在此添加自己的实现来报告文件名和行号 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
