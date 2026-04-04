/**
 ****************************************************************************************************
 * @file        gt9xxx.h
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

#include "string.h"
#include "stdio.h"
#include "lcd.h"
#include "touch.h"
#include "ctiic.h"
#include "gt9xxx.h"
#include "bsp_usart.h"
#include "bsp_delay.h"

#define delay_ms(x) HAL_Delay(x)


/* ע��: ����GT9271֧��10�㴥��֮��, ��������оƬֻ֧�� 5�㴥�� */
uint8_t g_gt_tnum = 5;      /* Ĭ��֧�ֵĴ���������(5�㴥��) */

/**
 * @brief       ��gt9xxxд��һ������
 * @param       reg : ��ʼ�Ĵ�����ַ
 * @param       buf : ���ݻ�������
 * @param       len : д���ݳ���
 * @retval      0, �ɹ�; 1, ʧ��;
 */
uint8_t gt9xxx_wr_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    uint8_t ret = 0;

    ct_iic_start();
    ct_iic_send_byte(GT9XXX_CMD_WR);    /* ����д���� */
    ct_iic_wait_ack();
    ct_iic_send_byte(reg >> 8);         /* ���͸�8λ��ַ */
    ct_iic_wait_ack();
    ct_iic_send_byte(reg & 0XFF);       /* ���͵�8λ��ַ */
    ct_iic_wait_ack();

    for (i = 0; i < len; i++)
    {
        ct_iic_send_byte(buf[i]);       /* ������ */
        ret = ct_iic_wait_ack();

        if (ret) break;
    }

    ct_iic_stop();  /* ����һ��ֹͣ���� */
    return ret;
}

/**
 * @brief       ��gt9xxx����һ������
 * @param       reg : ��ʼ�Ĵ�����ַ
 * @param       buf : ���ݻ�������
 * @param       len : �����ݳ���
 * @retval      ��
 */
void gt9xxx_rd_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    ct_iic_start();
    ct_iic_send_byte(GT9XXX_CMD_WR);    /* ����д���� */
    ct_iic_wait_ack();
    ct_iic_send_byte(reg >> 8);         /* ���͸�8λ��ַ */
    ct_iic_wait_ack();
    ct_iic_send_byte(reg & 0XFF);       /* ���͵�8λ��ַ */
    ct_iic_wait_ack();
    ct_iic_start();
    ct_iic_send_byte(GT9XXX_CMD_RD);    /* ���Ͷ����� */
    ct_iic_wait_ack();

    for (i = 0; i < len; i++)
    {
        buf[i] = ct_iic_read_byte(i == (len - 1) ? 0 : 1);  /* ��ȡ���� */
    }

    ct_iic_stop();  /* ����һ��ֹͣ���� */
}

/**
 * @brief       ��ʼ��gt9xxx������
 * @param       ��
 * @retval      0, ��ʼ���ɹ�; 1, ��ʼ��ʧ��;
 */
uint8_t gt9xxx_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    uint8_t temp[5];
    uint8_t retry = 0;

    printf("[GT9XXX] Starting init...\n");

    GT9XXX_RST_GPIO_CLK_ENABLE();
    GT9XXX_INT_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin = GT9XXX_RST_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GT9XXX_RST_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = GT9XXX_INT_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GT9XXX_INT_GPIO_PORT, &gpio_init_struct);

    ct_iic_init();

    for (retry = 0; retry < 3; retry++)
    {
        printf("[GT9XXX] Retry %d...\n", retry + 1);

        GT9XXX_RST(0);
        HAL_GPIO_WritePin(GT9XXX_INT_GPIO_PORT, GT9XXX_INT_GPIO_PIN, GPIO_PIN_RESET);
        delay_ms(20);
        GT9XXX_RST(1);
        delay_ms(50);

        gpio_init_struct.Pin = GT9XXX_INT_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_INPUT;
        gpio_init_struct.Pull = GPIO_NOPULL;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        HAL_GPIO_Init(GT9XXX_INT_GPIO_PORT, &gpio_init_struct);

        delay_ms(100);

        gt9xxx_rd_reg(GT9XXX_PID_REG, temp, 4);
        temp[4] = 0;

        printf("[GT9XXX] PID raw: [%02X %02X %02X %02X]\n", temp[0], temp[1], temp[2], temp[3]);

        if (temp[0] != 0xFF && temp[0] != 0x00) {
            break;
        }
        delay_ms(200);
    }

    if (strcmp((char *)temp, "911") == 0 ||
        strcmp((char *)temp, "9147") == 0 ||
        strcmp((char *)temp, "1158") == 0 ||
        strcmp((char *)temp, "9271") == 0)
    {
        printf("[GT9XXX] CTP ID:%s\n", temp);

        if (strcmp((char *)temp, "9271") == 0) {
            g_gt_tnum = 10;
        }

        temp[0] = 0X02;
        gt9xxx_wr_reg(GT9XXX_CTRL_REG, temp, 1);
        delay_ms(10);
        temp[0] = 0X00;
        gt9xxx_wr_reg(GT9XXX_CTRL_REG, temp, 1);

        return 0;
    }

    printf("[GT9XXX] Unknown chip ID or I2C failed\n");
    return 1;
}

/* GT9XXX 10��������(���) ��Ӧ�ļĴ����� */
const uint16_t GT9XXX_TPX_TBL[10] =
{
    GT9XXX_TP1_REG, GT9XXX_TP2_REG, GT9XXX_TP3_REG, GT9XXX_TP4_REG, GT9XXX_TP5_REG,
    GT9XXX_TP6_REG, GT9XXX_TP7_REG, GT9XXX_TP8_REG, GT9XXX_TP9_REG, GT9XXX_TP10_REG,
};

/**
 * @brief       ɨ�败����(���ò�ѯ��ʽ)
 * @param       mode : ������δ�õ��β���, Ϊ�˼��ݵ�����
 * @retval      ��ǰ����״̬
 *   @arg       0, �����޴���; 
 *   @arg       1, �����д���;
 */
uint8_t gt9xxx_scan(uint8_t mode)
{
    uint8_t buf[4];
    uint8_t i = 0;
    uint8_t res = 0;
    uint16_t temp;
    uint16_t tempsta;
    static uint8_t t = 0;   /* ���Ʋ�ѯ���,�Ӷ�����CPUռ���� */
    t++;

    if ((t % 10) == 0 || t < 10)    /* ����ʱ,ÿ����10��CTP_Scan�����ż��1��,�Ӷ���ʡCPUʹ���� */
    {
        gt9xxx_rd_reg(GT9XXX_GSTID_REG, &mode, 1);  /* ��ȡ�������״̬ */

        if ((mode & 0X80) && ((mode & 0XF) <= g_gt_tnum))
        {
            i = 0;
            gt9xxx_wr_reg(GT9XXX_GSTID_REG, &i, 1); /* ���־ */
        }

        if ((mode & 0XF) && ((mode & 0XF) <= g_gt_tnum))
        {
            temp = 0XFFFF << (mode & 0XF);  /* ����ĸ���ת��Ϊ1��λ��,ƥ��tp_dev.sta���� */
            tempsta = tp_dev.sta;           /* ���浱ǰ��tp_dev.staֵ */
            tp_dev.sta = (~temp) | TP_PRES_DOWN | TP_CATH_PRES;
            tp_dev.x[g_gt_tnum - 1] = tp_dev.x[0];  /* ���津��0������,���������һ���� */
            tp_dev.y[g_gt_tnum - 1] = tp_dev.y[0];

            for (i = 0; i < g_gt_tnum; i++)
            {
                if (tp_dev.sta & (1 << i))  /* ������Ч? */
                {
                    gt9xxx_rd_reg(GT9XXX_TPX_TBL[i], buf, 4);   /* ��ȡXY����ֵ */

                    if (lcddev.id == 0X5510 || lcddev.id == 0X9806 || lcddev.id == 0X7796)     /* 4.3��800*480 �� 3.5��480*320 MCU�� */
                    {
                        if (tp_dev.touchtype & 0X01)    /* ���� */
                        {
                            tp_dev.x[i] = lcddev.width - (((uint16_t)buf[3] << 8) + buf[2]);
                            tp_dev.y[i] = ((uint16_t)buf[1] << 8) + buf[0];
                        }
                        else
                        {
                            tp_dev.x[i] = ((uint16_t)buf[1] << 8) + buf[0];
                            tp_dev.y[i] = ((uint16_t)buf[3] << 8) + buf[2];
                        }
                    }
                    else    /* �����ͺ� */
                    {
                        if (tp_dev.touchtype & 0X01)    /* ���� */
                        {
                            tp_dev.x[i] = ((uint16_t)buf[1] << 8) + buf[0];
                            tp_dev.y[i] = ((uint16_t)buf[3] << 8) + buf[2];
                        }
                        else
                        {
                            tp_dev.x[i] = lcddev.width - (((uint16_t)buf[3] << 8) + buf[2]);
                            tp_dev.y[i] = ((uint16_t)buf[1] << 8) + buf[0];
                        }
                    }

                    //printf("x[%d]:%d,y[%d]:%d\r\n", i, tp_dev.x[i], i, tp_dev.y[i]);
                }
            }

            res = 1;

            if (tp_dev.x[0] > lcddev.width || tp_dev.y[0] > lcddev.height)  /* �Ƿ�����(���곬����) */
            {
                if ((mode & 0XF) > 1)   /* ��������������,�򸴵ڶ�����������ݵ���һ������. */
                {
                    tp_dev.x[0] = tp_dev.x[1];
                    tp_dev.y[0] = tp_dev.y[1];
                    t = 0;  /* ����һ��,��������������10��,�Ӷ���������� */
                }
                else        /* �Ƿ�����,����Դ˴�����(��ԭԭ����) */
                {
                    tp_dev.x[0] = tp_dev.x[g_gt_tnum - 1];
                    tp_dev.y[0] = tp_dev.y[g_gt_tnum - 1];
                    mode = 0X80;
                    tp_dev.sta = tempsta;   /* �ָ�tp_dev.sta */
                }
            }
            else 
            {
                t = 0;      /* ����һ��,��������������10��,�Ӷ���������� */
            }
        }
    }

    if ((mode & 0X8F) == 0X80)  /* �޴����㰴�� */
    {
        if (tp_dev.sta & TP_PRES_DOWN)      /* ֮ǰ�Ǳ����µ� */
        {
            tp_dev.sta &= ~TP_PRES_DOWN;    /* ��ǰ����ɿ� */
        }
        else    /* ֮ǰ��û�б����� */
        {
            tp_dev.x[0] = 0xffff;
            tp_dev.y[0] = 0xffff;
            tp_dev.sta &= 0XE000;           /* �������Ч��� */
        }
    }

    if (t > 240)t = 10; /* ���´�10��ʼ���� */

    return res;
}




























