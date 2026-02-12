#ifndef __BSP_DHT11_H__
#define __BSP_DHT11_H__

#include "main.h"
#include "bsp_delay.h"


/* IO操作函数 */
#define DHT11_DQ_OUT(x)     do{ x ? \
                                HAL_GPIO_WritePin(DHT11_DQ_GPIO_Port, DHT11_DQ_Pin, GPIO_PIN_SET) : \
                                HAL_GPIO_WritePin(DHT11_DQ_GPIO_Port, DHT11_DQ_Pin, GPIO_PIN_RESET); \
                            }while(0)                                                       /* 数据端口输出 */
#define DHT11_DQ_IN         HAL_GPIO_ReadPin(DHT11_DQ_GPIO_Port, DHT11_DQ_Pin)         /* 数据端口输入 */


uint8_t dht11_init(void);   /* 初始化DHT11 */
uint8_t dht11_check(void);  /* 检测是否存在DHT11 */
uint8_t dht11_read_data(uint8_t *temp,uint8_t *humi);   /* 读取温湿度 */

#endif /* __BSP_DHT11_H__ */
