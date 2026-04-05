/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
osMutexId_t lcd_mutex = NULL;
static osEventFlagsId_t app_events = NULL;
static osTimerId_t led_timer = NULL;
static osMutexId_t esp_mutex = NULL;

#define EVENT_WIFI_CONNECTED   (1 << 0)
#define EVENT_TIME_SYNCED      (1 << 1)
#define EVENT_MAIN_PAGE_READY  (1 << 2)

#define DEBUG_ENABLED  1

#if DEBUG_ENABLED
#define DEBUG_PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...)
#endif

/* WiFi Task */
osThreadId_t wifiTaskHandle;
const osThreadAttr_t wifiTask_attributes = {
  .name = "wifiTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Time Task */
osThreadId_t timeTaskHandle;
const osThreadAttr_t timeTask_attributes = {
  .name = "timeTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Sensor Task */
osThreadId_t sensorTaskHandle;
const osThreadAttr_t sensorTask_attributes = {
  .name = "sensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Weather Task */
osThreadId_t weatherTaskHandle;
const osThreadAttr_t weatherTask_attributes = {
    .name = "weatherTask",
    .stack_size = 512 * 4,
    .priority = (osPriority_t) osPriorityBelowNormal,
};

/* LVGL Task */
osThreadId_t lvglTaskHandle;
const osThreadAttr_t lvglTask_attributes = {
    .name = "lvglTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE END Variables */
/* Definitions for defaultTask */
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
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  lcd_mutex = osMutexNew(NULL);
  esp_mutex = osMutexNew(NULL);
  app_events = osEventFlagsNew(NULL);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
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
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
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
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
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

static void LedTimerCallback(void *argument)
{
    (void)argument;
    HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_9);
}

uint8_t time_is_synced(void)
{
    if (app_events == NULL) return 0;
    return (osEventFlagsGet(app_events) & EVENT_TIME_SYNCED) != 0;
}

uint8_t wifi_is_ready(void)
{
    if (app_events == NULL) return 0;
    return (osEventFlagsGet(app_events) & EVENT_WIFI_CONNECTED) != 0;
}

uint8_t main_page_is_ready(void)
{
    if (app_events == NULL) return 0;
    return (osEventFlagsGet(app_events) & EVENT_MAIN_PAGE_READY) != 0;
}

void lcd_lock(void)
{
    if (lcd_mutex != NULL)
    {
        osMutexAcquire(lcd_mutex, osWaitForever);
    }
}

void lcd_unlock(void)
{
    if (lcd_mutex != NULL)
    {
        osMutexRelease(lcd_mutex);
    }
}

void esp_lock(void)
{
    if (esp_mutex != NULL)
    {
        osMutexAcquire(esp_mutex, osWaitForever);
    }
}

void esp_unlock(void)
{
    if (esp_mutex != NULL)
    {
        osMutexRelease(esp_mutex);
    }
}

void StartWifiTask(void *argument)
{
    DEBUG_PRINTF("[WIFI_TASK] Starting...\n");
    
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    wifi_page_display();
    
    DEBUG_PRINTF("[WIFI_TASK] Initializing WiFi...\n");
    
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
    
    main_page_display();
    
    esp_wifi_info_t wifi_info = {0};
    if (esp_at_get_wifi_info(&wifi_info) && wifi_info.connected)
    {
        main_page_redraw_wifi_ssid(wifi_info.ssid, true);
    }
    
    osEventFlagsSet(app_events, EVENT_MAIN_PAGE_READY);
    DEBUG_PRINTF("[WIFI_TASK] Main page displayed, entering monitor loop\n");
    
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

void StartTimeTask(void *argument)
{
    DEBUG_PRINTF("[TIME_TASK] Starting...\n");
    
    DEBUG_PRINTF("[TIME_TASK] Waiting for WiFi...\n");
    osEventFlagsWait(app_events, EVENT_WIFI_CONNECTED, osFlagsWaitAll, osWaitForever);
    
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    DEBUG_PRINTF("[TIME_TASK] WiFi connected, syncing time...\n");
    
    for (;;)
    {
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

void StartSensorTask(void *argument)
{
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

void StartWeatherTask(void *argument)
{
    DEBUG_PRINTF("[WEATHER_TASK] Starting...\n");
    
    DEBUG_PRINTF("[WEATHER_TASK] Waiting for WiFi...\n");
    osEventFlagsWait(app_events, EVENT_WIFI_CONNECTED, osFlagsWaitAll, osWaitForever);
    
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    DEBUG_PRINTF("[WEATHER_TASK] WiFi connected, fetching weather...\n");
    
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

/* USER CODE END Application */

void StartLvglTask(void *argument)
{
    DEBUG_PRINTF("[LVGL_TASK] Starting...\n");
    for (;;)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
