/**
 * @file bsp_rtc.c
 * @brief RTC实时时钟驱动模块
 * 
 * 本文件实现了STM32F4内部RTC（实时时钟）的驱动功能，包括：
 * - 时间设置与读取
 * - 日期设置与读取
 * - 防抖动读取机制
 * 
 * RTC特性：
 * - 使用内部32.768kHz低速外部晶振(LSE)作为时钟源
 * - 支持年、月、日、星期、时、分、秒的完整时间信息
 * - 具有后备电池供电能力，主电源断电后仍可保持计时
 * 
 * 读取机制说明：
 * STM32F4的RTC采用双缓冲机制，读取时间和日期有特定顺序要求：
 * - 必须先读取时间寄存器，再读取日期寄存器
 * - 读取时间寄存器会触发日期寄存器的影子寄存器锁存
 * 
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#include "bsp_rtc.h"
#include "rtc.h"
#include <string.h>

/*============================================================================*/
/*                             外部变量声明                                   */
/*============================================================================*/

extern RTC_HandleTypeDef hrtc;    /**< CubeMX生成的RTC句柄 */

/*============================================================================*/
/*                             私有函数实现                                   */
/*============================================================================*/

/**
 * @brief 单次设置RTC时间（内部函数）
 * 
 * 将时间日期数据写入RTC寄存器，使用BIN格式（HAL库自动处理BCD转换）
 * 
 * @param date_time 指向时间日期结构体的指针
 * 
 * @note 此函数不包含防抖动逻辑，仅供内部使用
 * @note 年份存储时减去2000，因为STM32 RTC寄存器只保存0-99
 */
static void _bsp_rtc_set_time_once(const rtc_date_time_t *date_time)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    sTime.Hours = date_time->hour;
    sTime.Minutes = date_time->minute;
    sTime.Seconds = date_time->second;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    sDate.WeekDay = date_time->weekday;
    sDate.Month = date_time->month;
    sDate.Date = date_time->day;
    sDate.Year = date_time->year - 2000;

    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F2);
}

/**
 * @brief 单次读取RTC时间（内部函数）
 * 
 * 从RTC寄存器读取时间日期数据
 * 
 * @param date_time 输出的时间日期结构体指针
 * 
 * @note 必须先读Time再读Date，这是F4硬件锁存机制的要求
 * @note 读取后年份需要加上2000恢复完整年份
 */
static void _bsp_rtc_get_time_once(rtc_date_time_t *date_time)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    date_time->year = 2000 + sDate.Year;
    date_time->month = sDate.Month;
    date_time->day = sDate.Date;
    date_time->weekday = sDate.WeekDay;
    date_time->hour = sTime.Hours;
    date_time->minute = sTime.Minutes;
    date_time->second = sTime.Seconds;
}

/*============================================================================*/
/*                             公共函数实现                                   */
/*============================================================================*/

/**
 * @brief 设置RTC时间
 * 
 * 将指定的时间日期写入RTC寄存器，采用防抖动机制确保写入正确。
 * 
 * 防抖动原理：
 * 循环写入并读取验证，直到读取的秒数与写入的秒数一致。
 * 这可以防止在写入过程中发生秒进位导致的时间偏差。
 * 
 * @param date_time 指向时间日期结构体的指针
 * 
 * @note 此函数会阻塞直到写入成功
 * @note 通常在1-2次循环内即可完成
 */
void bsp_rtc_set_time(const rtc_date_time_t *date_time)
{
    rtc_date_time_t rtime;
    do {
        _bsp_rtc_set_time_once(date_time);
        _bsp_rtc_get_time_once(&rtime);
    } while (date_time->second != rtime.second);
}

/**
 * @brief 读取RTC时间
 * 
 * 从RTC寄存器读取当前时间日期，采用防抖动机制确保读取正确。
 * 
 * 防抖动原理：
 * 连续读取两次时间，直到两次读取结果完全一致。
 * 这可以防止在读取过程中发生进位（如23:59:59 -> 00:00:00）导致的数据不一致。
 * 
 * @param date_time 输出的时间日期结构体指针
 * 
 * @note 此函数会阻塞直到读取稳定
 * @note 通常在1-2次循环内即可完成
 */
void bsp_rtc_get_time(rtc_date_time_t *date_time)
{
    rtc_date_time_t time1, time2;
    do {
        _bsp_rtc_get_time_once(&time1);
        _bsp_rtc_get_time_once(&time2);
    } while (memcmp(&time1, &time2, sizeof(rtc_date_time_t)) != 0);
    
    memcpy(date_time, &time1, sizeof(rtc_date_time_t));
}
