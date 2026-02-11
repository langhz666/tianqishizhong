#include "bsp_rtc.h"
#include "rtc.h"      // 包含 CubeMX 生成的 rtc.h 以获取 hrtc 句柄
#include <string.h>

// 引用 CubeMX 在 rtc.c 中生成的句柄
extern RTC_HandleTypeDef hrtc;

/**
 * @brief  内部静态函数：单次设置时间 (移植为 HAL 库)
 */
static void _bsp_rtc_set_time_once(const rtc_date_time_t *date_time)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // 1. 填充时间结构体
    sTime.Hours = date_time->hour;
    sTime.Minutes = date_time->minute;
    sTime.Seconds = date_time->second;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    // 2. 填充日期结构体
    sDate.WeekDay = date_time->weekday; // HAL库定义: RTC_WEEKDAY_MONDAY = 1
    sDate.Month = date_time->month;
    sDate.Date = date_time->day;
    sDate.Year = date_time->year - 2000; // STM32 RTC 寄存器只保存 0-99

    // 3. 写入 RTC (使用 BIN 格式，HAL库会自动处理 BCD 转换)
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    // 4. (可选) 写入备份寄存器标记，表示时间已设置
    // 0x32F2 是一个魔法数字，下次上电可以读取它来判断是否需要重置时间
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F2);
}

/**
 * @brief  内部静态函数：单次获取时间 (移植为 HAL 库)
 */
static void _bsp_rtc_get_time_once(rtc_date_time_t *date_time)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // 1. 读取 RTC
    // 注意：必须先读 Time 再读 Date，这是 F4 硬件锁存机制的要求
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    // 2. 转换回自定义结构体
    date_time->year = 2000 + sDate.Year;
    date_time->month = sDate.Month;
    date_time->day = sDate.Date;
    date_time->weekday = sDate.WeekDay;
    date_time->hour = sTime.Hours;
    date_time->minute = sTime.Minutes;
    date_time->second = sTime.Seconds;
}

/**
 * @brief  对外接口：设置时间
 * (保留原逻辑：循环直到读出的秒数等于写入的秒数，确保写入成功)
 */
void bsp_rtc_set_time(const rtc_date_time_t *date_time)
{
    rtc_date_time_t rtime;
    // 这里的逻辑非常好，防止正在写入时发生秒进位导致设置偏差
    do {
        _bsp_rtc_set_time_once(date_time);
        _bsp_rtc_get_time_once(&rtime);
    } while (date_time->second != rtime.second);
}

/**
 * @brief  对外接口：获取时间
 * (保留原逻辑：连续读取两次直到一致，防止读取过程中发生进位)
 */
void bsp_rtc_get_time(rtc_date_time_t *date_time)
{
    rtc_date_time_t time1, time2;
    // 防抖动读取
    do {
        _bsp_rtc_get_time_once(&time1);
        _bsp_rtc_get_time_once(&time2);
    } while (memcmp(&time1, &time2, sizeof(rtc_date_time_t)) != 0);
    
    memcpy(date_time, &time1, sizeof(rtc_date_time_t));
}
