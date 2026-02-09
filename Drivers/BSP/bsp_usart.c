#include "bsp_usart.h"



/**
 * @description: 封装后的发送函数
 * @param {char} *str
 * @return {*}
 */
void USR_UART_Write(const char *str)
{
    if (str == NULL) return; // 简单的安全检查
    
    uint16_t len = strlen(str);
    // 使用 HAL 库发送，超时时间设为最大，模仿阻塞效果
    HAL_UART_Transmit(&huart1, (uint8_t *)str, len, HAL_MAX_DELAY);
}

/**
 * @description: 重定向 printf 到 UART1
 * @param {int} file
 * @param {char} *ptr
 * @param {int} len
 * @return {*}
 */
int _write(int file, char *ptr, int len)
{
    // 将 printf 的缓冲区数据通过串口发送
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}