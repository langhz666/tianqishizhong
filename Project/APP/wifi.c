/**
 * @file wifi.c
 * @brief WiFi连接管理模块
 * 
 * 本文件实现了WiFi模块的初始化和连接管理功能，包括：
 * - ESP AT指令模块初始化
 * - WiFi网络连接
 * - SNTP时间同步初始化
 * - 连接状态监控
 * 
 * 依赖：
 * - ESP8266/ESP32 AT固件
 * - HAL库延时函数
 * 
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#include "main.h"
#include <stdio.h>
#include <string.h>

#include "bsp_espat.h"
#include "page.h"
#include "app.h"

/**
 * @brief WiFi硬件和协议栈初始化
 * 
 * @return uint8_t  1: 初始化成功  0: 初始化失败
 * 
 * 初始化流程：
 * 1. ESP AT模块初始化（串口通信、AT指令测试）
 * 2. WiFi协议栈初始化
 * 3. SNTP时间同步服务初始化
 * 
 * 任何一个步骤失败都会导致整个初始化失败
 */
uint8_t wifi_init(void)
{
    if (!esp_at_init())
    {
        printf("[AT] init failed\n");
        return 0;
    }
    printf("[AT] inited\n");
    
    if (!esp_at_wifi_init())
    {
        printf("[WIFI] init failed\n");
        return 0;
    }
    printf("[WIFI] inited\n");
    
    if (!esp_at_sntp_init())
    {
        printf("[SNTP] init failed\n");
        return 0;
    }
    printf("[SNTP] inited\n");
    
    return 1;
}

/**
 * @brief 发起WiFi连接并等待结果（阻塞式，带超时）
 * 
 * 此函数会阻塞程序执行，直到连接成功或超时（10秒）。
 * 连接成功后返回，连接失败则显示错误页面并进入死循环。
 * 
 * @note WiFi SSID和密码在app.h中定义
 * @note 超时后会调用error_page_display显示错误信息
 */
void wifi_wait_connect(void)
{
    printf("[WIFI] connecting to %s...\n", WIFI_SSID);
    
    esp_at_connect_wifi(WIFI_SSID, WIFI_PASSWD, NULL);
    
    uint32_t start_tick = HAL_GetTick();
    
    while (HAL_GetTick() - start_tick < 10000)
    {
        HAL_Delay(500);
        
        esp_wifi_info_t wifi = { 0 };
        
        if (esp_at_get_wifi_info(&wifi) && wifi.connected)
        {
            printf("[WIFI] Connected Successfully!\n");
            printf("[WIFI] SSID: %s, BSSID: %s, Channel: %d, RSSI: %d\n",
                wifi.ssid, wifi.bssid, wifi.channel, wifi.rssi);
            return;
        }
        
        printf("[WIFI] waiting...\n");
    }
    
    printf("[WIFI] Connection Timeout\n");
    error_page_display("wireless connect failed");
    
    while (1)
    {
        HAL_Delay(100);
    }
}
