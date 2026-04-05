/**
 * @file bsp_dht11.c
 * @brief DHT11温湿度传感器驱动模块
 *
 * 本文件实现了与DHT11数字温湿度传感器通信的驱动程序。
 * DHT11是一款单总线数字传感器，可同时测量温度和湿度值。
 *
 * 硬件连接：
 * - VCC: 3.3V电源
 * - GND: 地
 * - DQ:  数据引脚（连接至MCU的可配置GPIO）
 *
 * 通信协议（DHT11单总线时序）：
 * 1. MCU发送起始信号：拉低DQ至少18ms，然后释放
 * 2. DHT11响应信号：先拉低80us，再拉高80us
 * 3. DHT11发送40位数据：高位在前，每位数据以50us低电平开始
 *    - 高电平26-28us表示数据0
 *    - 高电平70us表示数据1
 *
 * 数据格式（40位）：
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
 * @brief       复位DHT11传感器
 *
 * 发送复位脉冲以启动与DHT11的通信：
 * 1. MCU将DQ拉低并保持至少18ms（DHT11检测起始信号）
 * 2. MCU释放DQ（拉高）并等待20-40us
 * 3. DHT11将拉低DQ作为响应信号的开始
 *
 * @retval      无
 *
 * @note        调用此函数后应紧跟dht11_check()检查DHT11是否正常响应
 */
static void dht11_reset(void)
{
    DHT11_DQ_OUT(0);    /* MCU拉低DQ引脚 */
    HAL_Delay(20);       /* 保持低电平至少18ms */
    DHT11_DQ_OUT(1);    /* MCU释放DQ，置为高电平 */
    delay_us(30);       /* 等待20-40us后DHT11会拉低DQ作为响应 */
}

/**
 * @brief       检测DHT11是否存在并正常响应
 *
 * 检查DHT11是否正确连接并能正常通信：
 * 1. 等待DHT11拉低DQ（响应信号的第1部分：拉低40-80us）
 * 2. 等待DHT11拉高DQ（响应信号的第2部分：拉高40-80us）
 * 3. 如果任一步骤超时，返回错误
 *
 * @retval      0  DHT11正常存在
 * @retval      1  DHT11异常或不存在
 *
 * @note        此函数应在dht11_reset()之后调用
 */
uint8_t dht11_check(void)
{
    uint8_t retry = 0;
    uint8_t rval = 0;

    /* 等待DHT11拉低DQ（响应信号第1部分） */
    while (DHT11_DQ_IN && retry < 100)  /* DHT11会拉低40~80us */
    {
        retry++;
        delay_us(1);
    }

    if (retry >= 100)
    {
        rval = 1;  /* 超时，DHT11未响应 */
    }
    else
    {
        retry = 0;

        /* 等待DHT11拉高DQ（响应信号第2部分） */
        while (!DHT11_DQ_IN && retry < 100) /* DHT11拉低后会再次拉高40~80us */
        {
            retry++;
            delay_us(1);
        }
        if (retry >= 100) rval = 1;  /* 超时，DHT11响应异常 */
    }

    return rval;
}

/**
 * @brief       从DHT11读取一位数据
 *
 * DHT11数据位的时序：
 * - 每位数据以50us低电平开始
 * - 高电平26-28us表示数据0
 * - 高电平70us表示数据1
 *
 * @retval      0  读取到的位值为0
 * @retval      1  读取到的位值为1
 *
 * @note        此函数由dht11_read_byte()调用，不单独使用
 */
uint8_t dht11_read_bit(void)
{
    uint8_t retry = 0;

    /* 等待数据位起始的低电平结束 */
    while (DHT11_DQ_IN && retry < 100)  /* 等待变为低电平 */
    {
        retry++;
        delay_us(1);
    }

    retry = 0;

    /* 等待变为高电平（数据开始） */
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
 * @brief       从DHT11读取一个字节数据
 *
 * 按位循环读取8位数据，高位在前（MSB）
 *
 * @retval      读取到的字节值（0-255）
 *
 * @note        此函数由dht11_read_data()调用，不单独使用
 */
static uint8_t dht11_read_byte(void)
{
    uint8_t i, data = 0;

    for (i = 0; i < 8; i++)         /* 循环读取8位数据 */
    {
        data <<= 1;                 /* 高位数据先输出，先左移一位 */
        data |= dht11_read_bit();   /* 读取1bit数据并合并到data */
    }

    return data;
}

/**
 * @brief       从DHT11读取一次完整的温湿度数据
 *
 * 执行完整的读取流程：
 * 1. 发送复位信号
 * 2. 检查DHT11响应
 * 3. 读取5字节数据（湿度整数、湿度小数、温度整数、温度小数、校验和）
 * 4. 校验数据完整性
 * 5. 提取温度和湿度值
 *
 * @param[out]  temp    温度值指针，输出范围0~50℃
 * @param[out]  humi    湿度值指针，输出范围20%~90%
 *
 * @retval      0       读取成功
 * @retval      1       读取失败（校验失败或数据超范围）
 *
 * @note        温度和湿度的小数部分被丢弃（DHT11精度为整数）
 * @note        数据校验采用加法和校验
 */
uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];    /* 存储5字节数据 */
    uint8_t i;

    dht11_reset();     /* 发送复位信号 */

    if (dht11_check() == 0)  /* 检查DHT11响应 */
    {
        /* 读取40位（5字节）数据 */
        for (i = 0; i < 5; i++)
        {
            buf[i] = dht11_read_byte();
        }

        /* 校验：前4字节之和应等于第5字节（校验和） */
        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            /* 检查数据范围是否合理 */
            if (buf[2] >= 0 && buf[2] <= 50 && buf[0] >= 20 && buf[0] <= 90)
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
 * @brief       初始化DHT11温湿度传感器
 *
 * 配置DHT11数据引脚GPIO，并检测传感器是否存在。
 * GPIO配置为开漏输出模式，配合外部上拉电阻实现单总线通信。
 *
 * @retval      0       初始化成功，DHT11存在
 * @retval      1       初始化失败，DHT11不存在或通信错误
 *
 * @note        DHT11初始化时被注释掉的代码是因为GPIO已在gpio.c中预先配置
 * @note        初始化成功后即可调用dht11_read_data()读取温湿度数据
 */
uint8_t dht11_init(void)
{
    dht11_reset();     /* 发送复位信号 */
    return dht11_check();  /* 检查DHT11是否存在 */
}