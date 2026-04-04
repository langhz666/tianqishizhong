/**
 ****************************************************************************************************
 * @file        CT_IIC.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2021-10-25
 * @brief       ���ݴ����� ��������
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� ̽���� F407������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20211025
 * ��һ�η���
 *
 ****************************************************************************************************
 */

#ifndef __CT_IIC_H
#define __CT_IIC_H

#include "stm32f4xx_hal.h"


/******************************************************************************************/
/* CT_IIC ���� ���� */

#define CT_IIC_SCL_GPIO_PORT            GPIOB
#define CT_IIC_SCL_GPIO_PIN             GPIO_PIN_0
#define CT_IIC_SCL_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)   /* PB��ʱ��ʹ�� */

#define CT_IIC_SDA_GPIO_PORT            GPIOF
#define CT_IIC_SDA_GPIO_PIN             GPIO_PIN_11
#define CT_IIC_SDA_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)   /* PB��ʱ��ʹ�� */

/******************************************************************************************/

/* IO���� */
#define CT_IIC_SCL(x)     do{ x ? \
                              HAL_GPIO_WritePin(CT_IIC_SCL_GPIO_PORT, CT_IIC_SCL_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(CT_IIC_SCL_GPIO_PORT, CT_IIC_SCL_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)       /* SCL */

#define CT_IIC_SDA(x)     do{ x ? \
                              HAL_GPIO_WritePin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)       /* SDA */

#define CT_READ_SDA       HAL_GPIO_ReadPin(CT_IIC_SDA_GPIO_PORT, CT_IIC_SDA_GPIO_PIN) /* ��ȡSDA */


/* IIC���в������� */
void ct_iic_init(void);             /* ��ʼ��IIC��IO�� */
void ct_iic_stop(void);             /* ����IICֹͣ�ź� */
void ct_iic_start(void);            /* ����IIC��ʼ�ź� */

void ct_iic_ack(void);              /* IIC����ACK�ź� */
void ct_iic_nack(void);             /* IIC������ACK�ź� */
uint8_t ct_iic_wait_ack(void);      /* IIC�ȴ�ACK�ź� */

void ct_iic_send_byte(uint8_t txd);         /* IIC����һ���ֽ� */
uint8_t ct_iic_read_byte(unsigned char ack);/* IIC��ȡһ���ֽ� */

#endif







