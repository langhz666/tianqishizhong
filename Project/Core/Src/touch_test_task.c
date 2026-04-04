#include "FreeRTOS.h"
#include "task.h"
#include "touch.h"
#include "lvgl.h"
#include "cmsis_os2.h"
#include "bsp_delay.h"
#include <stdio.h>

extern osMutexId_t lcd_mutex;

static lv_obj_t *touch_coord_label = NULL;
static lv_obj_t *touch_test_label = NULL;
static lv_obj_t *touch_btn = NULL;
static uint32_t click_count = 0;

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        click_count++;
        if (touch_test_label) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Click: %lu", (unsigned long)click_count);
            lv_label_set_text(touch_test_label, buf);
        }
    }
}

void StartTouchTestTask(void *argument)
{
    printf("[TOUCH_TEST] Starting...\n");

    vTaskDelay(pdMS_TO_TICKS(1000));

    uint8_t ret = tp_init();
    printf("[TOUCH_TEST] tp_init returned: %d\n", ret);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    lv_obj_set_style_bg_color(scr, lv_color_make(0x22, 0x22, 0x22), LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Touch Test");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -100);

    touch_coord_label = lv_label_create(scr);
    lv_label_set_text(touch_coord_label, "X: --- Y: ---");
    lv_obj_set_style_text_color(touch_coord_label, lv_color_make(0x00, 0xFF, 0x00), LV_PART_MAIN);
    lv_obj_set_style_text_font(touch_coord_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(touch_coord_label, LV_ALIGN_CENTER, 0, -50);

    touch_test_label = lv_label_create(scr);
    lv_label_set_text(touch_test_label, "Click: 0");
    lv_obj_set_style_text_color(touch_test_label, lv_color_make(0xFF, 0xCC, 0x00), LV_PART_MAIN);
    lv_obj_set_style_text_font(touch_test_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(touch_test_label, LV_ALIGN_CENTER, 0, 0);

    touch_btn = lv_btn_create(scr);
    lv_obj_set_size(touch_btn, 120, 50);
    lv_obj_align(touch_btn, LV_ALIGN_CENTER, 0, 60);
    lv_obj_add_event_cb(touch_btn, btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *btn_label = lv_label_create(touch_btn);
    lv_label_set_text(btn_label, "TAP ME");
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(btn_label);

    printf("[TOUCH_TEST] UI created\n");

    uint32_t loop_count = 0;
    for (;;)
    {
        tp_dev.scan(0);

        if (loop_count % 20 == 0) {
            printf("[TOUCH_TEST] sta=0x%04X x=%u y=%u\n", tp_dev.sta, tp_dev.x[0], tp_dev.y[0]);
        }

        if (touch_coord_label) {
            char buf[32];
            snprintf(buf, sizeof(buf), "X: %u  Y: %u", tp_dev.x[0], tp_dev.y[0]);
            lv_label_set_text(touch_coord_label, buf);
        }

        loop_count++;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
