/**
 * @file weather.h
 * @brief 天气数据解析模块头文件
 *
 * 本头文件定义了天气信息的数据结构和解析函数接口。
 * 支持心知天气(Seniverse)API的JSON响应格式。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __WEATHER_H__
#define __WEATHER_H__

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 天气信息结构体
 *
 * 存储从天气API返回的解析后数据，包括位置信息和当前天气状况
 */
typedef struct
{
    char city[32];          /**< 城市名称，如"衡阳" */
    char location[128];     /**< 详细位置路径，如"衡阳,湖南,中国" */
    char weather[16];       /**< 天气描述，如"晴"、"多云" */
    int weather_code;       /**< 天气代码，用于图标映射 */
    int temperature;        /**< 当前温度(摄氏度) */
    int humidity;           /**< 湿度(百分比) */
    int wind_speed;         /**< 风速(km/h) */
    char wind_direction[8]; /**< 风向，如"北"、"东南" */
} weather_info_t;

/**
 * @brief 解析心知天气API的JSON响应
 *
 * @param response  JSON响应字符串指针
 * @param info      输出天气信息结构体指针
 * @return true     解析成功
 * @return false    解析失败
 */
bool parse_seniverse_response(const char *response, weather_info_t *info);

#endif /* __WEATHER_H__ */
