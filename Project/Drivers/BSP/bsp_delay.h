#ifndef __BSP_DELAY_H
#define __BSP_DELAY_H

#include "stm32f4xx_hal.h"


void DWT_Delay_Init(void);

void delay_us(uint32_t us);


#endif
