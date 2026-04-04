/**
 * @file page.h
 * @brief LVGL页面管理模块头文件
 * 
 * 本头文件声明了LVGL页面管理相关的函数接口，包括：
 * - 启动画面控制
 * - WiFi连接页面
 * - 主页面显示与数据刷新
 * 
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __PAGE_H__
#define __PAGE_H__

#include "bsp_rtc.h"

/*============================================================================*/
/*                             启动画面函数                                   */
/*============================================================================*/

/**
 * @brief 创建并显示启动画面
 */
void splash_screen_start(void);

/**
 * @brief 更新启动画面进度
 * @param progress  进度值 (0-100)
 * @param status    状态文本
 */
void splash_set_progress(uint8_t progress, const char *status);

/**
 * @brief 结束启动画面
 */
void splash_screen_end(void);

/*============================================================================*/
/*                             页面显示函数                                   */
/*============================================================================*/

/**
 * @brief 显示欢迎页面
 */
void welcome_page_display(void);

/**
 * @brief 显示错误页面
 * @param msg  错误信息字符串
 */
void error_page_display(const char *msg);

/**
 * @brief 显示WiFi连接页面
 */
void wifi_page_display(void);

/**
 * @brief 更新WiFi连接状态
 * @param status   状态文本
 * @param success  是否连接成功
 */
void wifi_page_set_status(const char *status, bool success);

/**
 * @brief 显示主页面
 */
void main_page_display(void);

/*============================================================================*/
/*                             数据刷新函数                                   */
/*============================================================================*/

/**
 * @brief 更新WiFi SSID显示
 * @param ssid  SSID字符串
 */
void main_page_redraw_wifi_ssid(const char *ssid);

/**
 * @brief 更新时间显示
 * @param time  RTC时间结构体指针
 */
void main_page_redraw_time(rtc_date_time_t *time);

/**
 * @brief 更新日期显示
 * @param date  RTC日期结构体指针
 */
void main_page_redraw_date(rtc_date_time_t *date);

/**
 * @brief 更新室内温度显示
 * @param temperature  温度值(摄氏度)
 */
void main_page_redraw_inner_temperature(float temperature);

/**
 * @brief 更新室内湿度显示
 * @param humidity  湿度值(百分比)
 */
void main_page_redraw_inner_humidity(float humidity);

/**
 * @brief 更新城市名称显示
 * @param city  城市名称
 */
void main_page_redraw_outdoor_city(const char *city);

/**
 * @brief 更新室外温度显示
 * @param temperature  温度值(摄氏度)
 */
void main_page_redraw_outdoor_temperature(float temperature);

/**
 * @brief 更新天气图标显示
 * @param code  天气代码
 */
void main_page_redraw_outdoor_weather_icon(const int code);

/**
 * @brief 更新天气湿度显示
 * @param humidity  湿度值(百分比)
 */
void main_page_redraw_outdoor_humidity(int humidity);

/**
 * @brief 更新天气风速显示
 * @param wind_speed  风速值(km/h)
 * @param wind_direction  风向字符串
 */
void main_page_redraw_outdoor_wind(int wind_speed, const char *wind_direction);

/*============================================================================*/
/*                             初始化函数                                     */
/*============================================================================*/

/**
 * @brief 初始化LVGL用户界面
 */
void lvgl_ui_init(void);

#endif /* __PAGE_H__ */
