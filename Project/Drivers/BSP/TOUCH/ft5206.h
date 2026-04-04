/**
 ****************************************************************************************************
 * @file        ft5206.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.1
 * @date        2023-05-29
 * @brief       7����ݴ�����-FT5206/FT5426 ��������
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
 * V1.1 20230529
 * ����7�� CST340������
 ****************************************************************************************************
 */
 
#ifndef __FT5206_H
#define __FT5206_H

#include "stm32f4xx_hal.h"


/******************************************************************************************/
/* FT5206 INT �� RST ���� ���� */

#define FT5206_RST_GPIO_PORT            GPIOC
#define FT5206_RST_GPIO_PIN             GPIO_PIN_13
#define FT5206_RST_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   /* PC��ʱ��ʹ�� */

#define FT5206_INT_GPIO_PORT            GPIOB
#define FT5206_INT_GPIO_PIN             GPIO_PIN_1
#define FT5206_INT_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)   /* PB��ʱ��ʹ�� */

/******************************************************************************************/

/* ����ݴ��������ӵ�оƬ����(δ����IIC����) 
 * IO�������� 
 */
#define FT5206_RST(x)     do{ x ? \
                              HAL_GPIO_WritePin(FT5206_RST_GPIO_PORT, FT5206_RST_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(FT5206_RST_GPIO_PORT, FT5206_RST_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)       /* ��λ���� */

#define FT5206_INT        HAL_GPIO_ReadPin(FT5206_INT_GPIO_PORT, FT5206_INT_GPIO_PIN)     /* ��ȡ�������� */

/* IIC��д���� */
#define FT5206_CMD_WR               0X70        /* д����(���λΪ0) */
#define FT5206_CMD_RD               0X71        /* ������(���λΪ1) */

/* FT5206 ���ּĴ�������  */
#define FT5206_DEVIDE_MODE          0x00        /* FT5206ģʽ���ƼĴ��� */
#define FT5206_REG_NUM_FINGER       0x02        /* ����״̬�Ĵ��� */

#define FT5206_TP1_REG              0X03        /* ��һ�����������ݵ�ַ */
#define FT5206_TP2_REG              0X09        /* �ڶ������������ݵ�ַ */
#define FT5206_TP3_REG              0X0F        /* ���������������ݵ�ַ */
#define FT5206_TP4_REG              0X15        /* ���ĸ����������ݵ�ַ */
#define FT5206_TP5_REG              0X1B        /* ��������������ݵ�ַ */ 


#define	FT5206_ID_G_LIB_VERSION     0xA1        /* �汾 */
#define FT5206_ID_G_MODE            0xA4        /* FT5206�ж�ģʽ���ƼĴ��� */
#define FT5206_ID_G_THGROUP         0x80        /* ������Чֵ���üĴ��� */
#define FT5206_ID_G_PERIODACTIVE    0x88        /* ����״̬�������üĴ��� */


uint8_t ft5206_wr_reg(uint16_t reg,uint8_t *buf,uint8_t len);
void ft5206_rd_reg(uint16_t reg,uint8_t *buf,uint8_t len);
uint8_t ft5206_init(void);
uint8_t ft5206_scan(uint8_t mode);

#endif

















