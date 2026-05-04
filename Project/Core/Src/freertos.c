/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : freertos.c
  * @brief          : FreeRTOS任务管理与系统核心逻辑
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp_usart.h"
#include "bsp_dht11.h"
#include "bsp_delay.h"
#include "bsp_rtc.h"
#include "bsp_espat.h"
#include "lcd.h"
#include "weather.h"
#include "page.h"
#include "app.h"
#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "lvgl.h"
#include "lv_port_indev.h"

extern int __io_putchar(int ch);
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
/* USER CODE BEGIN Variables */

/*============================================================================*/
/*                             同步对象定义                                   */
/*============================================================================*/

osMutexId_t lcd_mutex = NULL;           /**< LVGL互斥锁，保护LVGL线程安全 */
static osMutexId_t esp_mutex = NULL;    /**< ESP模块互斥锁，保护AT指令通信 */
static osEventFlagsId_t app_events = NULL;  /**< 应用事件标志组 */
static osTimerId_t led_timer = NULL;    /**< LED心跳定时器 */

/*============================================================================*/
/*                             事件标志定义                                   */
/*============================================================================*/

#define EVENT_WIFI_CONNECTED   (1 << 0)    /**< WiFi已连接事件 */
#define EVENT_TIME_SYNCED      (1 << 1)    /**< 时间已同步事件 */
#define EVENT_MAIN_PAGE_READY  (1 << 2)    /**< 主页面已就绪事件 */

/*============================================================================*/
/*                             调试配置                                       */
/*============================================================================*/

#define DEBUG_ENABLED  1

#if DEBUG_ENABLED
#define DEBUG_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...)
#endif

/*============================================================================*/
/*                             任务句柄与属性                                 */
/*============================================================================*/

/* WiFi任务 */
osThreadId_t wifiTaskHandle;
const osThreadAttr_t wifiTask_attributes = {
  .name = "wifiTask",
  .stack_size = 1024 * 4,       /* 4KB栈空间 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* 时间任务 */
osThreadId_t timeTaskHandle;
const osThreadAttr_t timeTask_attributes = {
  .name = "timeTask",
  .stack_size = 256 * 4,        /* 1KB栈空间 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* 传感器任务 */
osThreadId_t sensorTaskHandle;
const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
  .stack_size = 256 * 4,        /* 1KB栈空间 */
  .priority = (osPriority_t) osPriorityNormal,
};

/* 天气任务 */
osThreadId_t weatherTaskHandle;
const osThreadAttr_t weatherTask_attributes = {
    .name = "weatherTask",
    .stack_size = 512 * 4,      /* 2KB栈空间 */
    .priority = (osPriority_t) osPriorityBelowNormal,  /* 低于Normal，避免阻塞其他任务 */
};

/* LVGL任务 */
osThreadId_t lvglTaskHandle;
const osThreadAttr_t lvglTask_attributes = {
    .name = "lvglTask",
    .stack_size = 1024 * 4,     /* 4KB栈空间 */
    .priority = (osPriority_t) osPriorityNormal,
};

/* USER CODE END Variables */
/* 默认任务（CubeMX生成） */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartWifiTask(void *argument);
void StartTimeTask(void *argument);
void StartSensorTask(void *argument);
void StartWeatherTask(void *argument);
static void LedTimerCallback(void *argument);
void StartLvglTask(void *argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS初始化函数
  *
  * 创建所有同步对象（互斥锁、事件标志）和任务线程
  *
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  lcd_mutex = osMutexNew(NULL);      /* LVGL互斥锁 */
  esp_mutex = osMutexNew(NULL);      /* ESP模块互斥锁 */
  app_events = osEventFlagsNew(NULL); /* 应用事件标志组 */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* LED心跳定时器：每500ms切换一次LED状态 */
  osTimerAttr_t led_timer_attr = {
      .name = "ledTimer"
  };
  led_timer = osTimerNew(LedTimerCallback, osTimerPeriodic, NULL, &led_timer_attr);
  if (led_timer != NULL)
  {
      osTimerStart(led_timer, 500);
  }
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* USER CODE END RTOS_QUEUES */

  /* 创建任务线程 */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  wifiTaskHandle = osThreadNew(StartWifiTask, NULL, &wifiTask_attributes);
  timeTaskHandle = osThreadNew(StartTimeTask, NULL, &timeTask_attributes);
  sensorTaskHandle = osThreadNew(StartSensorTask, NULL, &sensorTask_attributes);
  weatherTaskHandle = osThreadNew(StartWeatherTask, NULL, &weatherTask_attributes);

  lvglTaskHandle = osThreadNew(StartLvglTask, NULL, &lvglTask_attributes);
  DEBUG_PRINTF("[FREERTOS] lvglTask created: %p\n", (void*)lvglTaskHandle);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  默认任务实现
  * @param  argument: 未使用
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  DEBUG_PRINTF("[DEFAULT_TASK] Starting...\n");
  for(;;)
  {
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/*============================================================================*/
/*                             系统服务函数                                   */
/*============================================================================*/

/**
 * @brief LED心跳定时器回调
 *
 * 每500ms切换一次PF9引脚（LED0），作为系统运行指示
 */
static void LedTimerCallback(void *argument)
{
    (void)argument;
    HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_9);
}

/**
 * @brief 检查时间是否已同步
 * @return 1: 已同步  0: 未同步
 */
uint8_t time_is_synced(void)
{
    if (app_events == NULL) return 0;
    return (osEventFlagsGet(app_events) & EVENT_TIME_SYNCED) != 0;
}

/**
 * @brief 检查WiFi是否已连接
 * @return 1: 已连接  0: 未连接
 */
uint8_t wifi_is_ready(void)
{
    if (app_events == NULL) return 0;
    return (osEventFlagsGet(app_events) & EVENT_WIFI_CONNECTED) != 0;
}

/**
 * @brief 检查主页面是否就绪
 * @return 1: 就绪  0: 未就绪
 */
uint8_t main_page_is_ready(void)
{
    if (app_events == NULL) return 0;
    return (osEventFlagsGet(app_events) & EVENT_MAIN_PAGE_READY) != 0;
}

/**
 * @brief 获取LCD互斥锁（阻塞等待）
 */
void lcd_lock(void)
{
    if (lcd_mutex != NULL)
    {
        osMutexAcquire(lcd_mutex, osWaitForever);
    }
}

/**
 * @brief 释放LCD互斥锁
 */
void lcd_unlock(void)
{
    if (lcd_mutex != NULL)
    {
        osMutexRelease(lcd_mutex);
    }
}

/**
 * @brief 获取ESP模块互斥锁（阻塞等待）
 */
void esp_lock(void)
{
    if (esp_mutex != NULL)
    {
        osMutexAcquire(esp_mutex, osWaitForever);
    }
}

/**
 * @brief 释放ESP模块互斥锁
 */
void esp_unlock(void)
{
    if (esp_mutex != NULL)
    {
        osMutexRelease(esp_mutex);
    }
}

/*============================================================================*/
/*                             WiFi任务                                       */
/*============================================================================*/

/**
 * @brief WiFi管理任务
 *
 * 职责：
 * 1. 初始化ESP模块和WiFi协议栈（最多重试3次）
 * 2. 连接到配置的WiFi热点
 * 3. 显示主页面并设置事件标志
 * 4. 持续监控WiFi连接状态，断线自动重连
 *
 * @param argument 未使用
 */
void StartWifiTask(void *argument)
{
    (void)argument;
    DEBUG_PRINTF("[WIFI_TASK] Starting...\n");

    vTaskDelay(pdMS_TO_TICKS(5000));  /* 等待系统稳定 */

    wifi_page_display();  /* 显示WiFi连接页面 */

    DEBUG_PRINTF("[WIFI_TASK] Initializing WiFi...\n");

    /* 初始化WiFi（最多重试3次） */
    uint8_t init_retry = 0;
    while (!wifi_init() && init_retry < 3)
    {
        init_retry++;
        DEBUG_PRINTF("[WIFI_TASK] WiFi init failed, retry %d/3...\n", init_retry);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    if (init_retry >= 3)
    {
        DEBUG_PRINTF("[WIFI_TASK] WiFi init failed after 3 retries\n");
        error_page_display("wireless init failed");
        for (;;){ vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    /* 连接到WiFi热点 */
    DEBUG_PRINTF("[WIFI_TASK] Connecting to %s...\n", WIFI_SSID);

    while (1)
    {
        if (esp_at_connect_wifi(WIFI_SSID, WIFI_PASSWD, NULL))
        {
            DEBUG_PRINTF("[WIFI_TASK] WiFi Connected, waiting for network stable...\n");
            vTaskDelay(pdMS_TO_TICKS(2000));

            esp_wifi_info_t wifi = {0};
            if (esp_at_get_wifi_info(&wifi))
            {
                DEBUG_PRINTF("[WIFI_TASK] WiFi info: connected=%d, SSID=%s\n", wifi.connected, wifi.ssid);
            }

            osEventFlagsSet(app_events, EVENT_WIFI_CONNECTED);
            break;
        }
        else
        {
            DEBUG_PRINTF("[WIFI_TASK] WiFi Connect failed, retry in 3s...\n");
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }

    /* 显示主页面 */
    main_page_display();

    esp_wifi_info_t wifi_info = {0};
    if (esp_at_get_wifi_info(&wifi_info) && wifi_info.connected)
    {
        main_page_redraw_wifi_ssid(wifi_info.ssid, true);
    }

    osEventFlagsSet(app_events, EVENT_MAIN_PAGE_READY);
    DEBUG_PRINTF("[WIFI_TASK] Main page displayed, entering monitor loop\n");

    /* WiFi连接监控循环 */
    uint8_t disconnect_count = 0;

    for (;;)
    {
        esp_wifi_info_t wifi = {0};
        if (esp_at_get_wifi_info(&wifi))
        {
            if (!wifi.connected)
            {
                disconnect_count++;
                if (disconnect_count >= 3)
                {
                    DEBUG_PRINTF("[WIFI_TASK] Connection lost, reconnecting...\n");
                    osEventFlagsClear(app_events, EVENT_WIFI_CONNECTED);
                    main_page_redraw_wifi_ssid("disconnected", false);

                    if (esp_at_connect_wifi(WIFI_SSID, WIFI_PASSWD, NULL))
                    {
                        DEBUG_PRINTF("[WIFI_TASK] Reconnected\n");
                        osEventFlagsSet(app_events, EVENT_WIFI_CONNECTED);
                        disconnect_count = 0;
                        esp_wifi_info_t new_wifi = {0};
                        if (esp_at_get_wifi_info(&new_wifi) && new_wifi.connected)
                        {
                            main_page_redraw_wifi_ssid(new_wifi.ssid, true);
                        }
                    }
                }
            }
            else
            {
                disconnect_count = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/*============================================================================*/
/*                             时间任务                                       */
/*============================================================================*/

/**
 * @brief 时间管理任务
 *
 * 职责：
 * 1. 等待WiFi连接完成
 * 2. 通过SNTP同步网络时间到RTC
 * 3. 每秒更新主页面的时间和日期显示
 *
 * @param argument 未使用
 */
void StartTimeTask(void *argument)
{
    (void)argument;
    DEBUG_PRINTF("[TIME_TASK] Starting...\n");

    DEBUG_PRINTF("[TIME_TASK] Waiting for WiFi...\n");
    osEventFlagsWait(app_events, EVENT_WIFI_CONNECTED, osFlagsWaitAll, osWaitForever);

    vTaskDelay(pdMS_TO_TICKS(3000));

    DEBUG_PRINTF("[TIME_TASK] WiFi connected, syncing time...\n");

    for (;;)
    {
        /* 如果时间未同步，尝试从SNTP获取 */
        if (!time_is_synced())
        {
            esp_date_time_t esp_date = {0};
            esp_lock();
            bool ok = esp_at_sntp_get_time(&esp_date);
            esp_unlock();

            if (ok && esp_date.year >= 2020)
            {
                DEBUG_PRINTF("[TIME_TASK] SNTP: %04u-%02u-%02u %02u:%02u:%02u\n",
                    esp_date.year, esp_date.month, esp_date.day,
                    esp_date.hour, esp_date.minute, esp_date.second);

                /* 将SNTP时间写入RTC */
                rtc_date_time_t rtc_date = {0};
                rtc_date.year = esp_date.year;
                rtc_date.month = esp_date.month;
                rtc_date.day = esp_date.day;
                rtc_date.hour = esp_date.hour;
                rtc_date.minute = esp_date.minute;
                rtc_date.second = esp_date.second;
                rtc_date.weekday = esp_date.weekday;

                bsp_rtc_set_time(&rtc_date);
                osEventFlagsSet(app_events, EVENT_TIME_SYNCED);
                DEBUG_PRINTF("[TIME_TASK] Time synced\n");
            }
            else
            {
                DEBUG_PRINTF("[TIME_TASK] SNTP failed, retry in 5s...\n");
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }

        /* 更新时间显示 */
        if (time_is_synced())
        {
            rtc_date_time_t date;
            bsp_rtc_get_time(&date);

            if (date.year >= 2020)
            {
                main_page_redraw_time(&date);
                main_page_redraw_date(&date);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*============================================================================*/
/*                             传感器任务                                     */
/*============================================================================*/

/**
 * @brief 温湿度传感器任务
 *
 * 职责：
 * 1. 等待主页面就绪
 * 2. 每3秒读取DHT11传感器数据
 * 3. 更新主页面的室内温湿度显示
 *
 * @param argument 未使用
 */
void StartSensorTask(void *argument)
{
    (void)argument;
    DEBUG_PRINTF("[SENSOR_TASK] Starting...\n");

    DEBUG_PRINTF("[SENSOR_TASK] Waiting for main page ready...\n");
    osEventFlagsWait(app_events, EVENT_MAIN_PAGE_READY, osFlagsWaitAll, osWaitForever);
    DEBUG_PRINTF("[SENSOR_TASK] Main page ready, starting sensor monitoring\n");

    vTaskDelay(pdMS_TO_TICKS(2000));

    static uint8_t last_temp = 0;
    static uint8_t last_humid = 0;
    static uint8_t has_valid_data = 0;

    for (;;)
    {
        uint8_t temp = 0, humid = 0;

        if (dht11_read_data(&temp, &humid) == 0)
        {
            last_temp = temp;
            last_humid = humid;
            has_valid_data = 1;

            DEBUG_PRINTF("[SENSOR_TASK] T=%dC, H=%d%%\n", temp, humid);
            main_page_redraw_inner_temperature((float)temp);
            main_page_redraw_inner_humidity((float)humid);
        }
        else
        {
            DEBUG_PRINTF("[SENSOR_TASK] DHT11 read failed\n");

            if (has_valid_data)
            {
                DEBUG_PRINTF("[SENSOR_TASK] Using last valid: T=%dC, H=%d%%\n", last_temp, last_humid);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/*============================================================================*/
/*                             天气任务                                       */
/*============================================================================*/

/**
 * @brief 天气信息获取任务
 *
 * 职责：
 * 1. 等待WiFi连接完成
 * 2. 每60秒通过HTTP GET请求心知天气API
 * 3. 解析JSON响应并更新主页面的室外天气显示
 *
 * @param argument 未使用
 */
void StartWeatherTask(void *argument)
{
    (void)argument;
    DEBUG_PRINTF("[WEATHER_TASK] Starting...\n");

    DEBUG_PRINTF("[WEATHER_TASK] Waiting for WiFi...\n");
    osEventFlagsWait(app_events, EVENT_WIFI_CONNECTED, osFlagsWaitAll, osWaitForever);

    vTaskDelay(pdMS_TO_TICKS(5000));

    DEBUG_PRINTF("[WEATHER_TASK] WiFi connected, fetching weather...\n");

    /* 心知天气API URL */
    static const char *weather_url = "http://api.seniverse.com/v3/weather/daily.json?key=SMrYk_pYNmh3z37k5&location=Hengyang&language=en&unit=c&days=1";
    static weather_info_t last_weather = {0};

    for (;;)
    {
        DEBUG_PRINTF("[WEATHER_TASK] Fetching weather...\n");

        esp_lock();
        const char *response = esp_at_http_get(weather_url);
        esp_unlock();

        if (response != NULL)
        {
            DEBUG_PRINTF("[WEATHER_TASK] Response: %.200s...\n", response);

            const char *json_start = strchr(response, '{');
            if (json_start != NULL)
            {
                weather_info_t weather = {0};
                if (parse_seniverse_response(json_start, &weather))
                {
                    DEBUG_PRINTF("[WEATHER_TASK] Parsed: city=%s, weather=%s, temp=%d, code=%d\n",
                        weather.city, weather.weather, weather.temperature, weather.weather_code);

                    /* 仅当数据变化时更新UI */
                    if (memcmp(&last_weather, &weather, sizeof(weather_info_t)) != 0)
                    {
                        memcpy(&last_weather, &weather, sizeof(weather_info_t));
                        DEBUG_PRINTF("[WEATHER_TASK] %s, %s, %dC, code=%d, humidity=%d%%, wind=%s %d km/h\n",
                            weather.city, weather.weather, weather.temperature, weather.weather_code,
                            weather.humidity, weather.wind_direction, weather.wind_speed);

                        main_page_redraw_outdoor_temperature(weather.temperature);
                        main_page_redraw_outdoor_weather_icon(weather.weather_code);
                        main_page_redraw_outdoor_humidity(weather.humidity);
                        main_page_redraw_outdoor_wind(weather.wind_speed, weather.wind_direction);
                    }
                }
                else
                {
                    DEBUG_PRINTF("[WEATHER_TASK] Parse failed, JSON: %.100s\n", json_start);
                }
            }
            else
            {
                DEBUG_PRINTF("[WEATHER_TASK] No JSON found in response\n");
            }
        }
        else
        {
            DEBUG_PRINTF("[WEATHER_TASK] HTTP failed\n");
        }

        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

/*============================================================================*/
/*                             LVGL任务                                       */
/*============================================================================*/

/**
 * @brief LVGL图形处理任务
 *
 * 职责：每5ms调用一次lv_task_handler()，驱动LVGL的UI渲染和事件处理
 *
 * @param argument 未使用
 */
void StartLvglTask(void *argument)
{
    (void)argument;
    DEBUG_PRINTF("[LVGL_TASK] Starting...\n");
    for (;;)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

/* USER CODE END Application */
