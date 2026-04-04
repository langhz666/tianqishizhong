/**
 ****************************************************************************************************
 * @file        GT9XXX.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.1
 * @date        2023-05-29
 * @brief       4.3����ݴ�����-GT9xxx ��������
 *   @note      GTϵ�е��ݴ�����ICͨ������,������֧��: GT9147/GT917S/GT968/GT1151/GT9271 �ȶ���
 *              ����IC, ��Щ����IC��ID��һ��, ��������������Ҫ���κ��޸ļ���ͨ��������ֱ������
 *
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
 * 1, ������ST7796 3.5����/ILI9806 4.3����GT1151��֧��
 * 2, gt9xxx_init���������Ӵ���IC�ж����������ض�����IC�ͷ���1��ʾ��ʼ��ʧ��
 ****************************************************************************************************
 */

#ifndef __GT9XXX_H
#define __GT9XXX_H

#include "stm32f4xx_hal.h"


/******************************************************************************************/
/* GT9XXX INT �� RST ���� ���� */

#define GT9XXX_RST_GPIO_PORT            GPIOC
#define GT9XXX_RST_GPIO_PIN             GPIO_PIN_13
#define GT9XXX_RST_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)   /* PC��ʱ��ʹ�� */

#define GT9XXX_INT_GPIO_PORT            GPIOB
#define GT9XXX_INT_GPIO_PIN             GPIO_PIN_1
#define GT9XXX_INT_GPIO_CLK_ENABLE()    do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)   /* PB��ʱ��ʹ�� */

/******************************************************************************************/

/* ����ݴ��������ӵ�оƬ����(δ����IIC����) 
 * IO�������� 
 */
#define GT9XXX_RST(x)     do{ x ? \
                              HAL_GPIO_WritePin(GT9XXX_RST_GPIO_PORT, GT9XXX_RST_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(GT9XXX_RST_GPIO_PORT, GT9XXX_RST_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)       /* ��λ���� */

#define GT9XXX_INT        HAL_GPIO_ReadPin(GT9XXX_INT_GPIO_PORT, GT9XXX_INT_GPIO_PIN)     /* ��ȡ�������� */

/* IIC��д���� */
#define GT9XXX_CMD_WR       0XBA        /* д���� */
#define GT9XXX_CMD_RD       0XBB        /* ������ */

/* GT9XXX ���ּĴ�������  */
#define GT9XXX_CTRL_REG     0X8040      /* GT9XXX���ƼĴ��� */
#define GT9XXX_CFGS_REG     0X8047      /* GT9XXX������ʼ��ַ�Ĵ��� */
#define GT9XXX_CHECK_REG    0X80FF      /* GT9XXXУ��ͼĴ��� */
#define GT9XXX_PID_REG      0X8140      /* GT9XXX��ƷID�Ĵ��� */

#define GT9XXX_GSTID_REG    0X814E      /* GT9XXX��ǰ��⵽�Ĵ������ */
#define GT9XXX_TP1_REG      0X8150      /* ��һ�����������ݵ�ַ */
#define GT9XXX_TP2_REG      0X8158      /* �ڶ������������ݵ�ַ */
#define GT9XXX_TP3_REG      0X8160      /* ���������������ݵ�ַ */
#define GT9XXX_TP4_REG      0X8168      /* ���ĸ����������ݵ�ַ */
#define GT9XXX_TP5_REG      0X8170      /* ��������������ݵ�ַ */
#define GT9XXX_TP6_REG      0X8178      /* ���������������ݵ�ַ */
#define GT9XXX_TP7_REG      0X8180      /* ���߸����������ݵ�ַ */
#define GT9XXX_TP8_REG      0X8188      /* �ڰ˸����������ݵ�ַ */
#define GT9XXX_TP9_REG      0X8190      /* �ھŸ����������ݵ�ַ */
#define GT9XXX_TP10_REG     0X8198      /* ��ʮ�����������ݵ�ַ */
 
/******************************************************************************************/
/* �������� */

uint8_t gt9xxx_wr_reg(uint16_t reg,uint8_t *buf,uint8_t len);   /* ��gt9xxд������ */
void gt9xxx_rd_reg(uint16_t reg,uint8_t *buf,uint8_t len);      /* ��gt9xx��ȡ���� */
uint8_t gt9xxx_init(void);                                      /* ��ʼ��gt9xxx������ */
uint8_t gt9xxx_scan(uint8_t mode);                              /* ɨ�败���� */

#endif












