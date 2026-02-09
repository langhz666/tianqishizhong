#ifndef __BSP_KEY_H__
#define __BSP_KEY_H__ 

#include "main.h"
#include "stdbool.h"
#include "led.h"
#include "bsp_beep.h"

#define KEY_0 0
#define KEY_1 1
#define KEY_2 2
#define KEY_UP 3

#define KEY_NONE_PRESS 0
#define KEY0_PRESS     1
#define KEY1_PRESS     2
#define KEY2_PRESS     3
#define KEYUP_PRESS    4




bool key_read(uint8_t idx);

uint8_t Key_Scan(uint8_t mode);

#endif /* __BSP_KEY_H__ */


