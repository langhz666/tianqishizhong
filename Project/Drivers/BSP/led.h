#ifndef __LED_H__
#define __LED_H__

#include "main.h"




void led_set(uint8_t led, uint8_t state);

void led_on(uint8_t led);

void led_off(uint8_t led);

void led_all_off();

void led_breath(uint32_t period, uint32_t duty);

void led_toggle(uint8_t led);

#endif // __LED_H__


