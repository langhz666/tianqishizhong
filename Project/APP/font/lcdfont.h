/**
 ****************************************************************************************************
 * @file        lcdfont.h
 * @author      ??????????(ALIENTEK)
 * @version     V1.0
 * @date        2021-10-16
 * @brief       ????12*12,16*16,24*24,32*32 ????LCD??ASCII????
 * @license     Copyright (c) 2020-2032, ??????????????????????
 ****************************************************************************************************
 * @attention
 *
 * ?????:??????? ????? F407??????
 * ???????:www.yuanzige.com
 * ???????:www.openedv.com
 * ??????:www.alientek.com
 * ??????:openedv.taobao.com
 *
 * ??????
 * V1.0 20211016
 * ????¦Ç???
 *
 ****************************************************************************************************
 */

#ifndef __LCDFONT_H
#define __LCDFONT_H

#include <stdint.h>

// ==========================================
// 1. ??????? (Structs & Enums)
// ==========================================

// ?????§³???
typedef enum {
    FONT_SIZE_16 = 16,
    FONT_SIZE_24 = 24,
    FONT_SIZE_32 = 32,
    FONT_SIZE_48 = 48
} FONT_SIZE;

// ??????????ŽG??
typedef struct {
    uint8_t Index[3]; // ????UTF-8???? (3???)
    uint8_t Msk[32];  // ???????? (16x16)
} typFNT_GB16;

typedef struct {
    uint8_t Index[3];
    uint8_t Msk[72];  // ???????? (24x24)
} typFNT_GB24;

typedef struct {
    uint8_t Index[3];
    uint8_t Msk[128]; // ???????? (32x32)
} typFNT_GB32;

typedef struct {
    uint8_t Index[3];
    uint8_t Msk[288]; // ???????? (48x48)
} typFNT_GB48;

typedef struct {
    const char *name;
    const uint8_t *model;
} font_chinese_t;

typedef struct {
    uint16_t size;
    const font_chinese_t *chinese;
} font_t;

// ==========================================
// 2. ?????????? (extern)
// ?????????????? {0x00...} ?????????
// ==========================================

// ASCII ????
extern const uint8_t asc2_1206[95][12];
extern const uint8_t asc2_1608[95][16];
extern const uint8_t asc2_2010[95][30];
extern const uint8_t asc2_2412[95][36];
extern const uint8_t asc2_3216[95][64];
extern const uint8_t asc2_4824[95][144];
extern const uint8_t asc2_5427[95][189];
extern const uint8_t asc2_6432[95][256];
extern const uint8_t asc2_7638[95][380];

// ???? ????
extern const typFNT_GB16 cn_16x16[];
extern const typFNT_GB24 cn_24x24[];
extern const typFNT_GB32 cn_32x32[];
extern const typFNT_GB48 cn_48x48[];

#endif





















