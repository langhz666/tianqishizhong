#ifndef __IMAG_H
#define __IMAG_H

#include <stdint.h>

typedef struct
{
    uint16_t width;
    uint16_t height;
    const uint8_t *data;
} image_t;

extern const image_t img_meihua;
extern const image_t img_error;
extern const image_t img_wifi;
extern const image_t icon_wenduji;
extern const image_t icon_wifi;
extern const image_t icon_duoyun;
extern const image_t icon_leizhenyu;
extern const image_t icon_qing;
extern const image_t icon_yintian;
extern const image_t icon_yueliang;
extern const image_t icon_zhongxue;
extern const image_t icon_zhongyu;
extern const image_t icon_na;
extern const image_t img_chengpingan;

// extern const unsigned char gImage_imag_kkedg[]; // 声明数组
// extern const unsigned char gImage_imag_test[]; // 声明数组

#endif // ! 
