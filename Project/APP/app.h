/*
 * @Author: langhz666 3204498297@qq.com
 * @Date: 2026-02-16 16:48:34
 * @LastEditors: langhz666 3204498297@qq.com
 * @LastEditTime: 2026-02-19 20:08:54
 * @FilePath: \lvgl\APP\app.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef __APP_H__
#define __APP_H__

#define APP_VERSION "v1.0"
#define WIFI_SSID   "iQOO Neo8 Pro"
#define WIFI_PASSWD "lhz19719937532"

void wifi_init(void);
void wifi_wait_connect(void);

void main_loop_init(void);
void main_loop(void);

void lcd_lock(void);
void lcd_unlock(void);

uint8_t wifi_is_ready(void);
uint8_t time_is_synced(void);
uint8_t main_page_is_ready(void);

#endif /* __APP_H__ */
