#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void lv_port_indev_init(void);
lv_indev_t * lv_port_get_encoder_indev(void);
lv_indev_t * lv_port_get_touch_indev(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_INDEV_H*/
