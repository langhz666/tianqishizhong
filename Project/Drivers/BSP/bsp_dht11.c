/**
 * @file bsp_dht11.c
 * @brief DHT11温湿度传感器驱动模块
 *
 * 本文件实现了DHT11数字温湿度传感器的通信驱动。
 * DHT11是一种单总线数字传感器，可同时测量温度和湿度值。
 *
 * 硬件连接：
 * - VCC: 3.3V电源
 * - GND: 地
 * - DQ:  数据引脚（连接到MCU的任意GPIO引脚）
 *
 * 通信协议（DHT11时序）：
 * 1. MCU发送起始信号：拉低DQ至少18ms，然后释放
 * 2. DHT11应答信号：拉低80us，再拉高80us
 * 3. DHT11发送40位数据（高位在前），每位以50us低电平开始
 *    - 高电平26-28us表示数据0
 *    - 高电平70us表示数据1
 *
 * 数据格式：40位（5字节）
 * - 第1字节：湿度整数部分
 * - 第2字节：湿度小数部分（通常为0）
 * - 第3字节：温度整数部分
 * - 第4字节：温度小数部分（通常为0）
 * - 第5字节：校验和（前4字节之和）
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#include "bsp_dht11.h"

/**
 * @brief 复位DHT11传感器
 *
 * 发送复位信号以启动DHT11通信：
 * 1. MCU拉低DQ数据线至少18ms（DHT11起始信号）
 * 2. MCU释放DQ数据线（拉高），等待20-40us
 * 3. DHT11拉低DQ作为应答信号的开始
 *
 * @retval 无
 *
 * @note 调用此函数后应检查dht11_check()确认DHT11是否正常应答
 */
static void dht11_reset(void)
{
    DHT11_DQ_OUT(0);    /* MCU拉低DQ数据线 */
    HAL_Delay(20);       /* 保持低电平至少18ms */
    DHT11_DQ_OUT(1);    /* MCU释放DQ数据线为高电平 */
    delay_us(30);        /* 等待20-40us，DHT11拉低DQ作为应答 */
}

/**
 * @brief 检测DHT11是否正常存在并应答
 *
 * 检测DHT11是否正确响应硬件通信：
 * 1. 等待DHT11拉低DQ（应答信号的第1部分，持续40-80us）
 * 2. 等待DHT11拉高DQ（应答信号的第2部分，持续40-80us）
 * 3. 如果任一步骤超时，返回错误
 *
 * @retval 0  DHT11存在且正常
 * @retval 1  DHT11异常或不存在
 *
 * @note 此函数应在dht11_reset()之后调用
 */
uint8_t dht11_check(void)
{
    uint8_t retry = 0;
    uint8_t rval = 0;

    /* 等待DHT11拉低DQ（应答信号第1部分） */
    while (DHT11_DQ_IN && retry < 100)  /* DHT11拉低40~80us */
    {
        retry++;
        delay_us(1);
    }

    if (retry >= 100)
    {
        rval = 1;  /* 超时，DHT11未应答 */
    }
    else
    {
        retry = 0;

        /* 等待DHT11拉高DQ（应答信号第2部分） */
        while (!DHT11_DQ_IN && retry < 100) /* DHT11拉高后再拉低40~80us */
        {
            retry++;
            delay_us(1);
        }
        if (retry >= 100) rval = 1;  /* 超时，DHT11应答异常 */
    }

    return rval;
}

/**
 * @brief 从DHT11读取一位数据
 *
 * DHT11位数据时序：
 * - 每位数据以50us低电平开始
 * - 高电平26-28us表示数据0
 * - 高电平70us表示数据1
 *
 * @retval 0  读取的数据位值为0
 * @retval 1  读取的数据位值为1
 *
 * @note 此函数由dht11_read_byte()调用，用户无需直接使用
 */
uint8_t dht11_read_bit(void)
{
    uint8_t retry = 0;

    /* 等待数据位开始的低电平结束 */
    while (DHT11_DQ_IN && retry < 100)  /* 等待变为低电平 */
    {
        retry++;
        delay_us(1);
    }

    retry = 0;

    /* 等待变为高电平（数据开始传输） */
    while (!DHT11_DQ_IN && retry < 100) /* 等待变高电平 */
    {
        retry++;
        delay_us(1);
    }

    /* 等待40us后读取DQ状态判断数据是0还是1 */
    delay_us(40);

    if (DHT11_DQ_IN)    /* 高电平70us表示1，低电平26-28us表示0 */
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief 从DHT11读取一个字节数据
 *
 * 逐位循环读取8位数据，高位在前（MSB）
 *
 * @retval 读取到的字节值（0-255）
 *
 * @note 此函数由dht11_read_data()调用，用户无需直接使用
 */
static uint8_t dht11_read_byte(void)
{
    uint8_t i, data = 0;

    for (i = 0; i < 8; i++)         /* 循环读取8位数据 */
    {
        data <<= 1;                 /* 左移一位，为接收下一位腾出空间 */
        data |= dht11_read_bit();   /* 读取1bit数据并合并到data中 */
    }

    return data;
}

/**
 * @brief 从DHT11读取一次完整的温湿度数据
 *
 * 执行完整的读取流程：
 * 1. 发送复位信号
 * 2. 检测DHT11应答
 * 3. 读取5字节数据（湿度整数、湿度小数、温度整数、温度小数、校验和）
 * 4. 校验数据完整性
 * 5. 提取温度和湿度值
 *
 * @param[out]  temp    温度值指针，范围0~50
 * @param[out]  humi    湿度值指针，范围20%~90%
 *
 * @retval 0       读取成功
 * @retval 1       读取失败（校验失败或数据超范围）
 *
 * @note 温度和湿度的小数部分直接忽略（DHT11为整数传感器）
 * @note 校验和计算：前4字节之和应等于第5字节
 */
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];    /* 存储5字节数据 */
    uint8_t i;

    dht11_reset();     /* 发送复位信号 */

    if (dht11_check() == 0)  /* 检测DHT11应答 */
    {
        /* 读取40位（5字节）数据 */
        for (i = 0; i < 5; i++)
        {
            buf[i] = dht11_read_byte();
        }

        /* 校验：前4字节之和应等于第5字节（校验和） */
        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            /* 检查数据范围是否合法 */
            if (buf[2] <= 50 && buf[0] >= 20 && buf[0] <= 90)
            {
                *humi = buf[0];  /* 湿度整数部分 */
                *temp = buf[2];  /* 温度整数部分 */
                return 0;       /* 读取成功 */
            }
        }
    }

    return 1;  /* 读取失败 */
}

/**
 * @brief 初始化DHT11温湿度传感器
 *
 * 配置DHT11数据引脚并检测传感器是否存在。
 * GPIO配置为开漏输出模式，通过外部上拉电阻实现双向通信。
 *
 * @retval 0       初始化成功，DHT11存在并响应
 * @retval 1       初始化失败，DHT11不存在或通信错误
 *
 * @note DHT11初始化时需注意注释掉的代码行（因为GPIO配置已在gpio.c中预设完成）
 * @note 初始化成功后即可调用dht11_read_data()读取温湿度数据
 */
uint8_t dht11_init(void)
{
    dht11_reset();     /* 发送复位信号 */
    return dht11_check();  /* 检测DHT11是否存在 */
}
