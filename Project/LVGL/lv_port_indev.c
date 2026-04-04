#include "lv_port_indev.h"
#include "bsp_key.h"

static lv_indev_t * encoder_indev;
static int32_t encoder_diff;
static lv_indev_state_t encoder_state;

static uint8_t last_key_state = 0;
static uint8_t key_pressed = 0;

static uint8_t read_key_state(void)
{
    uint8_t state = 0;
    if (key_read(KEY_0)) state |= (1 << 0);
    if (key_read(KEY_1)) state |= (1 << 1);
    if (key_read(KEY_2)) state |= (1 << 2);
    if (key_read(KEY_UP)) state |= (1 << 3);
    return state;
}

static void encoder_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    uint8_t current_state = read_key_state();
    uint8_t changed = current_state ^ last_key_state;

    encoder_diff = 0;
    encoder_state = LV_INDEV_STATE_REL;

    if (changed & (1 << 0)) {
        if (current_state & (1 << 0)) {
            encoder_diff = -1;
            key_pressed = 1;
        }
    }

    if (changed & (1 << 1)) {
        if (current_state & (1 << 1)) {
            encoder_diff = 1;
            key_pressed = 1;
        }
    }

    if (changed & (1 << 3)) {
        if (current_state & (1 << 3)) {
            encoder_state = LV_INDEV_STATE_PR;
            key_pressed = 1;
        }
    }

    if (changed & (1 << 2)) {
        if (current_state & (1 << 2)) {
            encoder_diff = 0;
            encoder_state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_ESC;
            key_pressed = 1;
        }
    }

    if (key_pressed) {
        data->enc_diff = encoder_diff;
        data->state = encoder_state;
        key_pressed = 0;
    } else {
        data->enc_diff = 0;
        data->state = LV_INDEV_STATE_REL;
    }

    last_key_state = current_state;
}

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    indev_drv.read_cb = encoder_read;
    encoder_indev = lv_indev_drv_register(&indev_drv);
}

lv_indev_t * lv_port_get_encoder_indev(void)
{
    return encoder_indev;
}
