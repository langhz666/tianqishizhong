/**
 * @file imag.h
 * @brief 图片资源声明头文件
 *
 * 本头文件声明了项目中使用的图片资源结构体和外部变量。
 * 图片数据以C数组形式存储在对应的.c文件中，采用RGB565格式。
 *
 * @author Smart Weather Clock Team
 * @version 1.0.0
 */

#ifndef __IMAG_H__
#define __IMAG_H__

#include <stdint.h>

/**
 * @brief 图片资源结构体
 *
 * 描述一张图片的宽度、高度和像素数据指针
 */
typedef struct
{
    uint16_t width;         /**< 图片宽度（像素） */
    uint16_t height;        /**< 图片高度（像素） */
    const uint8_t *data;    /**< 像素数据指针（RGB565格式） */
} image_t;

/* 外部图片资源声明 */
extern const image_t img_chengpingan;           /**< 欢迎页Logo图片 */
extern const unsigned char gImage_xiaozhang[];  /**< 主页面背景图片 */

#endif /* __IMAG_H__ */
