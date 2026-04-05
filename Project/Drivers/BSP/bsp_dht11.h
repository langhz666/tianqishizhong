/**
 * @file bsp_dht11.h
 * @brief DHT11温湿度传感器驱动模块头文件
 *
 * 本头文件定义了DHT11传感器驱动的接口、数据类型和宏定义。
 * DHT11是一款低成本、高精度的数字温湿度传感器，适用于各类环境监测应用。
 *
 * 主要特性：
 * - 单总线通信协议（仅需一根数据线）
 * - 温度测量范围：0~50℃（精度±2℃）
 * - 湿度测量范围：20%~90%RH（精度±5%RH）
 * - 低功耗设计
 *
 * 硬件连接：
 * - VCC → 3.3V
 * - GND → GND
 * - DQ  → 可配置GPIO（开漏输出+外部上拉）
 *
 * 使用示例：
 * @code
 * uint8_t temp, humi;
 *
 * // 初始化传感器
 * dht11_init();
 *
 * // 读取温湿度数据
 * if (dht11_read_data(&temp, &humi) == 0)
 * {
 *     printf("温度: %d℃, 湿度: %d%%\n", temp, humi);
 * }
 * @endcode
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __BSP_DHT11_H__
#define __BSP_DHT11_H__

#include "main.h"          /* 引入HAL库定义和GPIO相关类型 */
#include "bsp_delay.h"      /* 引入微秒延时函数 */

/*============================================================================*/
/*                             IO操作宏定义                                   */
/*============================================================================*/

/**
 * @brief DHT11数据引脚输出控制宏
 *
 * 设置DQ引脚的输出状态，用于向DHT11发送数据或控制信号
 *
 * @param x  0=输出低电平，1=输出高电平（释放总线）
 *
 * @note 由于使用开漏模式，高电平输出实际是关闭输出让外部上拉电阻拉高
 */
#define DHT11_DQ_OUT(x)     do{ x ? \
                                HAL_GPIO_WritePin(DHT11_DQ_GPIO_Port, DHT11_DQ_Pin, GPIO_PIN_SET) : \
                                HAL_GPIO_WritePin(DHT11_DQ_GPIO_Port, DHT11_DQ_Pin, GPIO_PIN_RESET); \
                            }while(0)

/**
 * @brief DHT11数据引脚输入读取宏
 *
 * 读取DQ引脚的当前电平状态，用于接收DHT11的响应和数据
 *
 * @retval GPIO_PIN_RESET  低电平（0）
 * @retval GPIO_PIN_SET    高电平（1）
 */
#define DHT11_DQ_IN         HAL_GPIO_ReadPin(DHT11_DQ_GPIO_Port, DHT11_DQ_Pin)

/*============================================================================*/
/*                             函数接口声明                                   */
/*============================================================================*/

/**
 * @brief 初始化DHT11温湿度传感器
 *
 * 配置GPIO引脚并检测DHT11是否存在
 *
 * @retval 0  初始化成功，DHT11存在且正常
 * @retval 1  初始化失败，DHT11不存在或通信错误
 *
 * @note 应在系统初始化阶段调用此函数
 */
uint8_t dht11_init(void);

/**
 * @brief 检测DHT11传感器是否存在
 *
 * 向DHT11发送检测信号并等待响应，用于验证传感器连接
 *
 * @retval 0  DHT11正常存在
 * @retval 1  DHT11异常或不存在
 *
 * @note 此函数通常由dht11_init()内部调用
 */
uint8_t dht11_check(void);

/**
 * @brief 读取DHT11温湿度数据
 *
 * 执行完整的温湿度读取流程，包括信号复位、数据接收和校验
 *
 * @param[out] temp   温度值输出指针（范围：0~50℃）
 * @param[out] humi   湿度值输出指针（范围：20%~90%）
 *
 * @retval 0  读取成功，数据已写入temp和humi
 * @retval 1  读取失败（校验错误或数据超范围）
 *
 * @note 读取间隔应大于1秒，避免频繁读取导致数据异常
 * @note 返回的温度和湿度值为整数（舍弃小数部分）
 */
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi);

#endif /* __BSP_DHT11_H__ */