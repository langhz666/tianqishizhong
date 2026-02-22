#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "weather.h"

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
	
	const char *loaction_name_response = strstr(location_response, "\"name\":");
	if (loaction_name_response)
	{
		sscanf(loaction_name_response, "\"name\": \"%31[^\"]\"", info->city);
	}
	
	const char *loaction_path_response = strstr(location_response, "\"path\":");
	if (loaction_path_response)
	{
		sscanf(loaction_path_response, "\"path\": \"%128[^\"]\"", info->loaction);
	}
	
	const char *now_response = strstr(response, "\"now\":");
	if (now_response == NULL)
	{
		return false;
	}
	
	const char *now_text_response = strstr(now_response, "\"text\":");
	if (now_text_response)
	{
		sscanf(now_text_response, "\"text\": \"%15[^\"]\"", info->weather);
	}
	
	const char *now_code_response = strstr(now_response, "\"code\":");
	if (now_code_response)
	{
		sscanf(now_code_response, "\"code\": \"%d\"", &info->weather_code);
	}
	
	char temperature_str[16] = { 0 };
	const char *now_temperature_response = strstr(now_response, "\"temperature\":");
	if (now_temperature_response)
	{
		now_temperature_response += strlen("\"temperature\":\"");
		for (int i = 0; i < 15 && now_temperature_response[i] != '\0' && now_temperature_response[i] != '"'; i++)
		{
			temperature_str[i] = now_temperature_response[i];
		}
		if (strlen(temperature_str) > 0)
		{
			info->temperature = atoi(temperature_str);
		}
	}
	
	return true;
}
