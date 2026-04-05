/**
 * @file bsp_rtc.h
 * @brief RTC实时时钟驱动模块头文件
 * 
 * 本头文件定义了RTC驱动的数据结构和函数接口。
 * RTC（实时时钟）用于保持系统时间，即使主电源断电也能继续计时（需要后备电池）。
 * 
 * 主要功能：
 * - 设置系统时间（通常从网络同步）
 * - 读取当前时间用于显示
 * 
 * 使用示例：
 * @code
 * // 设置时间
 * rtc_date_time_t time = {
 *     .year = 2026,
 *     .month = 4,
 *     .day = 4,
 *     .weekday = 5,  // 周五
 *     .hour = 14,
 *     .minute = 30,
 *     .second = 0
 * };
 * bsp_rtc_set_time(&time);
 * 
 * // 读取时间
 * rtc_date_time_t current;
 * bsp_rtc_get_time(&current);
 * printf("Current: %04d-%02d-%02d %02d:%02d:%02d\n",
 *        current.year, current.month, current.day,
 *        current.hour, current.minute, current.second);
 * @endcode
 * 
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __BSP_RTC_H__
#define __BSP_RTC_H__

#include "main.h"

/*============================================================================*/
/*                             数据结构定义                                   */
/*============================================================================*/

/**
 * @brief 日期时间结构体
 * 
 * 存储完整的日期和时间信息，用于RTC的读写操作。
 * 所有字段均使用无符号整数，便于显示和处理。
 */
typedef struct {
    uint16_t year;      /**< 公历年份，如2026 */
    uint8_t month;      /**< 月份，范围1-12 */
    uint8_t day;        /**< 日期，范围1-31 */
    uint8_t weekday;    /**< 星期，范围1-7（1=周一，7=周日） */
    uint8_t hour;       /**< 小时，范围0-23（24小时制） */
    uint8_t minute;     /**< 分钟，范围0-59 */
    uint8_t second;     /**< 秒，范围0-59 */
} rtc_date_time_t;

/*============================================================================*/
/*                             函数声明                                       */
/*============================================================================*/

/**
 * @brief 设置RTC时间
 * 
 * 将指定的时间日期写入RTC寄存器，采用防抖动机制确保写入正确。
 * 
 * @param date_time 指向时间日期结构体的指针
 * 
 * @note 此函数会阻塞直到写入成功
 */
void bsp_rtc_set_time(const rtc_date_time_t *date_time);

/**
 * @brief 读取RTC时间
 * 
 * 从RTC寄存器读取当前时间日期，采用防抖动机制确保读取正确。
 * 
 * @param date_time 输出的时间日期结构体指针
 * 
 * @note 此函数会阻塞直到读取稳定
 */
void bsp_rtc_get_time(rtc_date_time_t *date_time);

#endif /* __BSP_RTC_H__ */
