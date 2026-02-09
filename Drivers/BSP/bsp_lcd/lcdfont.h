/**
 ****************************************************************************************************
 * @file        lcdfont.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-10-16
 * @brief       包含12*12,16*16,24*24,32*32 四种LCD用ASCII字体
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 探索者 F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20211016
 * 第一次发布
 *
 ****************************************************************************************************
 */

#ifndef __LCDFONT_H
#define __LCDFONT_H

#include <stdint.h>

// ==========================================
// 1. 类型定义 (Structs & Enums)
// ==========================================

// 字体大小枚举
typedef enum {
    FONT_SIZE_16 = 16,
    FONT_SIZE_24 = 24,
    FONT_SIZE_32 = 32,
    FONT_SIZE_48 = 48
} FONT_SIZE;

// 汉字字模结构体定义
typedef struct {
    uint8_t Index[3]; // 汉字UTF-8编码 (3字节)
    uint8_t Msk[32];  // 点阵数据 (16x16)
} typFNT_GB16;

typedef struct {
    uint8_t Index[3];
    uint8_t Msk[72];  // 点阵数据 (24x24)
} typFNT_GB24;

typedef struct {
    uint8_t Index[3];
    uint8_t Msk[128]; // 点阵数据 (32x32)
} typFNT_GB32;

typedef struct {
    uint8_t Index[3];
    uint8_t Msk[288]; // 点阵数据 (48x48)
} typFNT_GB48;

// ==========================================
// 2. 外部变量声明 (extern)
// 注意：这里绝对不要放 {0x00...} 这种数据！
// ==========================================

// ASCII 字体
extern const uint8_t asc2_1206[95][12];
extern const uint8_t asc2_1608[95][16];
extern const uint8_t asc2_2412[95][36];
extern const uint8_t asc2_3216[95][64];

// 中文 字体
extern const typFNT_GB16 cn_16x16[];
extern const typFNT_GB24 cn_24x24[];
extern const typFNT_GB32 cn_32x32[];
extern const typFNT_GB48 cn_48x48[];

#endif





















