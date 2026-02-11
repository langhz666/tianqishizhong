#ifndef __BSP_RTC_H__
#define __BSP_RTC_H__

#include "main.h" // 包含 main.h 以获取 HAL 库定义

// 自定义时间日期结构体（保持你原有的结构）
typedef struct {
    uint16_t year;    // 公历年份，如 2026
    uint8_t month;    // 1-12
    uint8_t day;      // 1-31
    uint8_t weekday;  // 1-7 (1=周一, 7=周日)
    uint8_t hour;     // 0-23
    uint8_t minute;   // 0-59
    uint8_t second;   // 0-59
} rtc_date_time_t;

// 函数声明
void bsp_rtc_set_time(const rtc_date_time_t *date_time);
void bsp_rtc_get_time(rtc_date_time_t *date_time);

#endif /* __BSP_RTC_H__ */
