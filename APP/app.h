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

#endif /* __APP_H__ */
