/**
 * @file page.h
 * @brief LVGLҳ�����ģ��ͷ�ļ�
 * 
 * ��ͷ�ļ�������LVGLҳ�������صĺ����ӿڣ�������
 * - �����������
 * - WiFi����ҳ��
 * - ��ҳ����ʾ������ˢ��
 * 
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __PAGE_H__
#define __PAGE_H__

#include "bsp_rtc.h"

/*============================================================================*/
/*                             �������溯��                                   */
/*============================================================================*/

/**
 * @brief ��������ʾ��������
 */
void splash_screen_start(void);

/**
 * @brief ���������������
 * @param progress  ����ֵ (0-100)
 * @param status    ״̬�ı�
 */
void splash_set_progress(uint8_t progress, const char *status);

/**
 * @brief ������������
 */
void splash_screen_end(void);

/*============================================================================*/
/*                             ҳ����ʾ����                                   */
/*============================================================================*/

/**
 * @brief ��ʾ��ӭҳ��
 */
void welcome_page_display(void);

/**
 * @brief ��ʾ����ҳ��
 * @param msg  ������Ϣ�ַ���
 */
void error_page_display(const char *msg);

/**
 * @brief ��ʾWiFi����ҳ��
 */
void wifi_page_display(void);

/**
 * @brief ����WiFi����״̬
 * @param status   ״̬�ı�
 * @param success  �Ƿ����ӳɹ�
 */
void wifi_page_set_status(const char *status, bool success);

/**
 * @brief ��ʾ��ҳ��
 */
void main_page_display(void);

/*============================================================================*/
/*                             ����ˢ�º���                                   */
/*============================================================================*/

/**
 * @brief WiFi SSIDʾ
 * @param ssid      SSIDַ
 * @param connected Ƿӳɹ
 */
void main_page_redraw_wifi_ssid(const char *ssid, bool connected);

/**
 * @brief ����ʱ����ʾ
 * @param time  RTCʱ��ṹ��ָ��
 */
void main_page_redraw_time(rtc_date_time_t *time);

/**
 * @brief ����������ʾ
 * @param date  RTC���ڽṹ��ָ��
 */
void main_page_redraw_date(rtc_date_time_t *date);

/**
 * @brief ���������¶���ʾ
 * @param temperature  �¶�ֵ(���϶�)
 */
void main_page_redraw_inner_temperature(float temperature);

/**
 * @brief ��������ʪ����ʾ
 * @param humidity  ʪ��ֵ(�ٷֱ�)
 */
void main_page_redraw_inner_humidity(float humidity);

/**
 * @brief ���³���������ʾ
 * @param city  ��������
 */
void main_page_redraw_outdoor_city(const char *city);

/**
 * @brief ���������¶���ʾ
 * @param temperature  �¶�ֵ(���϶�)
 */
void main_page_redraw_outdoor_temperature(float temperature);

/**
 * @brief ��������ͼ����ʾ
 * @param code  ��������
 */
void main_page_redraw_outdoor_weather_icon(const int code);

/**
 * @brief ��������ʪ����ʾ
 * @param humidity  ʪ��ֵ(�ٷֱ�)
 */
void main_page_redraw_outdoor_humidity(int humidity);

/**
 * @brief ��������������ʾ
 * @param wind_speed  ����ֵ(km/h)
 * @param wind_direction  �����ַ���
 */
void main_page_redraw_outdoor_wind(int wind_speed, const char *wind_direction);

/*============================================================================*/
/*                             ��ʼ������                                     */
/*============================================================================*/

/**
 * @brief ��ʼ��LVGL�û�����
 */
void lvgl_ui_init(void);

#endif /* __PAGE_H__ */
