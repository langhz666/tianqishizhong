/**
 ****************************************************************************************************
 * @file        ctiic.c
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
 
#include "ctiic.h"
#include "bsp_delay.h"


/**
 * @brief       ����I2C�ٶȵ���ʱ
 * @param       ��
 * @retval      ��
 */
static void ct_iic_delay(void)
{
    delay_us(2);
}

/**
 * @brief       ���ݴ���оƬIIC�ӿڳ�ʼ��
 * @param       ��
 * @retval      ��
 */
void ct_iic_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    
    CT_IIC_SCL_GPIO_CLK_ENABLE();   /* SCL����ʱ��ʹ�� */
    CT_IIC_SDA_GPIO_CLK_ENABLE();   /* SDA����ʱ��ʹ�� */

    gpio_init_struct.Pin = CT_IIC_SCL_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;             /* ��©��� */
    gpio_init_struct.Pull = GPIO_PULLUP;                     /* ���� */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;      /* ���� */
    HAL_GPIO_Init(CT_IIC_SCL_GPIO_PORT, &gpio_init_struct);  /* ��ʼ��SCL���� */
    
    gpio_init_struct.Pin = CT_IIC_SDA_GPIO_PIN;
    HAL_GPIO_Init(CT_IIC_SDA_GPIO_PORT, &gpio_init_struct);  /* ��ʼ��SDA���� */
    /* SDA����ģʽ����,��©���,����, �����Ͳ���������IO������, ��©�����ʱ��(=1), Ҳ���Զ�ȡ�ⲿ�źŵĸߵ͵�ƽ */

    ct_iic_stop();  /* ֹͣ�����������豸 */
}

/**
 * @brief       ����IIC��ʼ�ź�
 * @param       ��
 * @retval      ��
 */
void ct_iic_start(void)
{
    CT_IIC_SDA(1);
    CT_IIC_SCL(1);
    ct_iic_delay();
    CT_IIC_SDA(0);      /* START�ź�: ��SCLΪ��ʱ, SDA�Ӹ߱�ɵ�, ��ʾ��ʼ�ź� */
    ct_iic_delay();
    CT_IIC_SCL(0);      /* ǯסI2C���ߣ�׼�����ͻ�������� */
    ct_iic_delay();
}

/**
 * @brief       ����IICֹͣ�ź�
 * @param       ��
 * @retval      ��
 */
void ct_iic_stop(void)
{
    CT_IIC_SDA(0);      /* STOP�ź�: ��SCLΪ��ʱ, SDA�ӵͱ�ɸ�, ��ʾֹͣ�ź� */
    ct_iic_delay();
    CT_IIC_SCL(1);
    ct_iic_delay();
    CT_IIC_SDA(1);      /* ����I2C���߽����ź� */
    ct_iic_delay();
}

/**
 * @brief       �ȴ�Ӧ���źŵ���
 * @param       ��
 * @retval      1������Ӧ��ʧ��
 *              0������Ӧ��ɹ�
 */
uint8_t ct_iic_wait_ack(void)
{
    uint8_t waittime = 0;
    uint8_t rack = 0;
    
    CT_IIC_SDA(1);      /* �����ͷ�SDA��(��ʱ�ⲿ������������SDA��) */
    ct_iic_delay();
    CT_IIC_SCL(1);      /* SCL=1, ��ʱ�ӻ����Է���ACK */
    ct_iic_delay();

    while (CT_READ_SDA) /* �ȴ�Ӧ�� */
    {
        waittime++;

        if (waittime > 250)
        {
            ct_iic_stop();
            rack = 1;
            break;
        }

        ct_iic_delay();
    }

    CT_IIC_SCL(0);      /* SCL=0, ����ACK��� */
    ct_iic_delay();
    return rack;
}


/**
 * @brief       ����ACKӦ��
 * @param       ��
 * @retval      ��
 */
void ct_iic_ack(void)
{
    CT_IIC_SDA(0);  /* SCL 0 -> 1  ʱSDA = 0,��ʾӦ�� */
    ct_iic_delay();
    CT_IIC_SCL(1);
    ct_iic_delay();
    CT_IIC_SCL(0);
    ct_iic_delay(); 
    CT_IIC_SDA(1);  /* �����ͷ�SDA�� */
    ct_iic_delay(); 
}

/**
 * @brief       ������ACKӦ��
 * @param       ��
 * @retval      ��
 */
void ct_iic_nack(void)
{
    CT_IIC_SDA(1);  /* SCL 0 -> 1  ʱ SDA = 1,��ʾ��Ӧ�� */
    ct_iic_delay();
    CT_IIC_SCL(1);
    ct_iic_delay();
    CT_IIC_SCL(0);
    ct_iic_delay();
}

/**
 * @brief       IIC����һ���ֽ�
 * @param       data: Ҫ���͵�����
 * @retval      ��
 */
void ct_iic_send_byte(uint8_t data)
{
    uint8_t t;
    
    for (t = 0; t < 8; t++)
    {
        CT_IIC_SDA((data & 0x80) >> 7); /* ��λ�ȷ��� */
        ct_iic_delay();
        CT_IIC_SCL(1);
        ct_iic_delay();
        CT_IIC_SCL(0);
        data <<= 1;     /* ����1λ,������һ�η��� */
    }

    CT_IIC_SDA(1);      /* �������, �����ͷ�SDA�� */
}

/**
 * @brief       IIC����һ���ֽ�
 * @param       ack:  ack=1ʱ������ack; ack=0ʱ������nack
 * @retval      ���յ�������
 */
uint8_t ct_iic_read_byte(unsigned char ack)
{
    uint8_t i, receive = 0;

    for (i = 0; i < 8; i++ )    /* ����1���ֽ����� */
    {
        receive <<= 1;          /* ��λ�����,�������յ�������λҪ���� */
        CT_IIC_SCL(1);
        ct_iic_delay();

        if (CT_READ_SDA)
        {
            receive++;
        }
        
        CT_IIC_SCL(0);
        ct_iic_delay();

    }

    if (!ack)
    {
        ct_iic_nack();  /* ����nACK */
    }
    else
    {
        ct_iic_ack();   /* ����ACK */
    }

    return receive;
}







