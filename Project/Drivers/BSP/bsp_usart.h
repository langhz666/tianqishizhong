#ifndef __BSP_USART_H__
#define __BSP_USART_H__

#include "main.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>  

// 函数声明
void USR_UART_Write(const char *str);
int _write(int file, char *ptr, int len);

#endif // __BSP_USART_H__
