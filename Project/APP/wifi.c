#include "main.h"       // 必须包含，用于获取 HAL_Delay 和 HAL_GetTick
#include <stdio.h>
#include <string.h>

/* BSP 和应用头文件 */
#include "bsp_espat.h"  // 替换原来的 esp_at.h
#include "page.h"       // 用于 error_page_display
#include "app.h"        // 用于 WIFI_SSID 和 WIFI_PASSWD 定义

/**
 * @brief WiFi 硬件和协议栈初始化
 */
void wifi_init(void)
{
    // 1. 初始化 AT 串口底层
    if (!esp_at_init())
    {
        printf("[AT] init failed\n");
        goto err;
    }
    printf("[AT] inited\n");
    
    // 2. 初始化 WiFi 模式 (Station)
    if (!esp_at_wifi_init())
    {
        printf("[WIFI] init failed\n");
        goto err;
    }
    printf("[WIFI] inited\n");
    
    // 3. 初始化 SNTP (网络时间)
    if (!esp_at_sntp_init())
    {
        printf("[SNTP] init failed\n");
        goto err;
    }
    printf("[SNTP] inited\n");
    
    return;
    
err:
    // 初始化失败，显示错误页并死循环
    error_page_display("wireless init failed");
    while (1)
    {
        HAL_Delay(100); // 加一点延时防止死循环占满总线
    }
}

/**
 * @brief 发起连接并等待 (阻塞式，带超时)
 */
void wifi_wait_connect(void)
{
    printf("[WIFI] connecting to %s...\n", WIFI_SSID);
    
    // 发送连接命令
    // 注意：请确保 app.h 中定义了 WIFI_SSID 和 WIFI_PASSWD
    esp_at_connect_wifi(WIFI_SSID, WIFI_PASSWD, NULL);
    
    // 记录开始时间
    uint32_t start_tick = HAL_GetTick();
    
    // 循环检查连接状态，超时时间 10秒 (10000ms)
    while (HAL_GetTick() - start_tick < 10000)
    {
        HAL_Delay(500); // 每 500ms 检查一次
        
        esp_wifi_info_t wifi = { 0 };
        
        // 获取信息并判断是否已连接
        if (esp_at_get_wifi_info(&wifi) && wifi.connected)
        {
            printf("[WIFI] Connected Successfully!\n");
            printf("[WIFI] SSID: %s, BSSID: %s, Channel: %d, RSSI: %d\n",
                wifi.ssid, wifi.bssid, wifi.channel, wifi.rssi);
            return; // 连接成功，退出函数
        }
        
        printf("[WIFI] waiting...\n");
    }
    
    // 超时处理
    printf("[WIFI] Connection Timeout\n");
    error_page_display("wireless connect failed");
    
    while (1)
    {
        HAL_Delay(100);
    }
}
