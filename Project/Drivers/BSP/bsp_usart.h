/**
 * @file bsp_usart.h
 * @brief 串口驱动模块头文件
 *
 * 本头文件定义了串口发送和printf重定向的函数接口。
 * printf输出重定向到USART1（调试串口）。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __BSP_USART_H__
#define __BSP_USART_H__

#include "main.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 通过UART1发送字符串
 *
 * @param str 要发送的字符串指针
 */
void USR_UART_Write(const char *str);

/**
 * @brief 重定向printf到UART1
 *
 * @param file 文件描述符（未使用）
 * @param ptr 数据缓冲区指针
 * @param len 数据长度
 * @return int 发送的字节数
 */
int _write(int file, char *ptr, int len);

#endif /* __BSP_USART_H__ */
