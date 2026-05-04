/**
 * @file weather.c
 * @brief 天气数据解析模块
 *
 * 本文件实现了对心知天气(Seniverse)API返回的JSON数据进行解析的功能。
 * 解析后的数据存储到weather_info_t结构体中，供UI层使用。
 *
 * 心知天气Daily API返回格式示例：
 * {
 *   "results": [{
 *     "location": {
 *       "name": "衡阳",
 *       "path": "衡阳,湖南,中国"
 *     },
 *     "daily": [{
 *       "date": "2025-04-04",
 *       "text_day": "晴",
 *       "code_day": "0",
 *       "high": "26",
 *       "low": "15",
 *       "wind_direction": "北",
 *       "wind_speed": "15.0",
 *       "humidity": "60"
 *     }]
 *   }]
 * }
 *
 * @author Smart Weather Clock Team
 * @version 1.2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "weather.h"

/**
 * @brief 解析心知天气API的JSON响应
 *
 * @param response  JSON响应字符串指针
 * @param info      输出天气信息结构体指针
 * @return true     解析成功
 * @return false    解析失败（参数无效或格式错误）
 *
 * 解析流程：
 * 1. 验证参数有效性
 * 2. 定位"results"节点
 * 3. 解析location信息（城市名称、路径）
 * 4. 解析daily信息（天气描述、代码、温度、湿度、风速）
 *
 * @note 使用简单的字符串匹配方式解析JSON，无需JSON库
 *       适用于资源有限的嵌入式环境
 */
bool parse_seniverse_response(const char *response, weather_info_t *info)
{
    if (response == NULL || strlen(response) == 0)
    {
        return false;
    }

    response = strstr(response, "\"results\":");
    if (response == NULL)
    {
        return false;
    }

    const char *location_response = strstr(response, "\"location\":");
    if (location_response == NULL)
    {
        return false;
    }

    const char *location_name_response = strstr(location_response, "\"name\":");
    if (location_name_response)
    {
        sscanf(location_name_response, "\"name\": \"%31[^\"]\"", info->city);
    }

    const char *location_path_response = strstr(location_response, "\"path\":");
    if (location_path_response)
    {
        sscanf(location_path_response, "\"path\": \"%128[^\"]\"", info->location);
    }

    const char *daily_response = strstr(response, "\"daily\":");
    if (daily_response == NULL)
    {
        return false;
    }

    const char *daily_text_response = strstr(daily_response, "\"text_day\":");
    if (daily_text_response)
    {
        sscanf(daily_text_response, "\"text_day\": \"%15[^\"]\"", info->weather);
    }

    const char *daily_code_response = strstr(daily_response, "\"code_day\":");
    if (daily_code_response)
    {
        sscanf(daily_code_response, "\"code_day\": \"%d\"", &info->weather_code);
    }

    char high_str[16] = { 0 };
    const char *daily_high_response = strstr(daily_response, "\"high\":");
    if (daily_high_response)
    {
        sscanf(daily_high_response, "\"high\": \"%15[^\"]\"", high_str);
        if (strlen(high_str) > 0)
        {
            info->temperature = atoi(high_str);
        }
    }

    const char *daily_humidity_response = strstr(daily_response, "\"humidity\":");
    if (daily_humidity_response)
    {
        sscanf(daily_humidity_response, "\"humidity\": \"%d\"", &info->humidity);
    }

    char wind_speed_str[16] = { 0 };
    const char *daily_wind_speed_response = strstr(daily_response, "\"wind_speed\":");
    if (daily_wind_speed_response)
    {
        sscanf(daily_wind_speed_response, "\"wind_speed\": \"%15[^\"]\"", wind_speed_str);
        if (strlen(wind_speed_str) > 0)
        {
            info->wind_speed = atoi(wind_speed_str);
        }
    }

    const char *daily_wind_direction_response = strstr(daily_response, "\"wind_direction\":");
    if (daily_wind_direction_response)
    {
        sscanf(daily_wind_direction_response, "\"wind_direction\": \"%7[^\"]\"", info->wind_direction);
    }

    return true;
}
