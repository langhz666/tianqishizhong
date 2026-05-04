/**
 * @file bsp_usart.c
 * @brief 串口驱动模块
 *
 * 本文件实现了串口发送和printf重定向功能。
 * USART1配置为115200-8-N-1，用于调试信息输出。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#include "bsp_usart.h"

/**
 * @brief 通过UART1发送字符串
 *
 * @param str 要发送的字符串指针
 */
void USR_UART_Write(const char *str)
{
    if (str == NULL) return;

    uint16_t len = strlen(str);
    HAL_UART_Transmit(&huart1, (uint8_t *)str, len, HAL_MAX_DELAY);
}

/**
 * @brief 重定向printf到UART1
 *
 * 将printf的输出通过串口发送，实现调试信息打印。
 *
 * @param file 文件描述符（未使用）
 * @param ptr 数据缓冲区指针
 * @param len 数据长度
 * @return int 发送的字节数
 */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}
