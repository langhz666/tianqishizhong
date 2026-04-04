/**
 * @file lvgl_pages.c
 * @brief LVGL页面管理模块 - 智能桌面终端UI界面
 * 
 * 本文件实现了基于LVGL图形库的智能桌面终端用户界面，采用多页面滑动架构：
 * - 启动画面(Splash Screen)：显示Logo动画、进度条和状态信息
 * - WiFi连接页面：显示WiFi连接状态和进度
 * - 主界面：支持滑动切换的多页面系统
 *   - 页面1：时间显示（主页面）
 *   - 页面2：天气详情
 *   - 页面3：系统设置
 * 
 * 界面布局(240x320屏幕)：
 * +------------------+
 * |  状态栏 (24px)   |  WiFi图标/SSID
 * +------------------+
 * |                  |
 * |  滑动内容区      |
 * |  (276px)         |
 * |                  |
 * +------------------+
 * | 指示器 (20px)    |
 * +------------------+
 * 
 * @author Smart Weather Clock Team
 * @version 1.2.0
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "lcd.h"
#include "bsp_rtc.h"
#include "cmsis_os2.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "app.h"

/*============================================================================*/
/*                             布局常量定义                                   */
/*============================================================================*/

#define STATUS_BAR_HEIGHT   24      /**< 状态栏高度 */
#define INDICATOR_HEIGHT    20      /**< 底部指示器高度 */
#define CONTENT_HEIGHT      276     /**< 内容区高度 (320 - 24 - 20) */
#define SCREEN_WIDTH        240     /**< 屏幕宽度 */
#define SCREEN_HEIGHT       320     /**< 屏幕高度 */

/*============================================================================*/
/*                             外部变量声明                                    */
/*============================================================================*/

extern osMutexId_t lcd_mutex;

/*============================================================================*/
/*                             静态全局变量                                    */
/*============================================================================*/

/*---------------------------- 全局状态栏 ----------------------------------*/
static lv_obj_t *status_bar = NULL;             /**< 状态栏容器 */
static lv_obj_t *status_wifi_icon = NULL;       /**< WiFi图标 */
static lv_obj_t *status_wifi_label = NULL;      /**< WiFi SSID标签 */

/*---------------------------- 滑动视图组件 --------------------------------*/
static lv_obj_t *tabview = NULL;                /**< 滑动标签页容器 */
static lv_obj_t *page_indicator[3] = {NULL};   /**< 页面指示器点 */

/*---------------------------- 页面1：时间显示组件 ------------------------*/
static lv_obj_t *time_page = NULL;              /**< 时间显示页面容器 */
static lv_obj_t *time_label = NULL;             /**< 时间显示标签 */
static lv_obj_t *date_label = NULL;             /**< 日期显示标签 */
static lv_obj_t *indoor_card = NULL;            /**< 室内环境卡片 */
static lv_obj_t *indoor_temp_label = NULL;      /**< 室内温度标签 */
static lv_obj_t *indoor_humidity_label = NULL;  /**< 室内湿度标签 */
static lv_obj_t *outdoor_card = NULL;           /**< 室外环境卡片 */
static lv_obj_t *outdoor_temp_label = NULL;     /**< 室外温度标签 */
static lv_obj_t *weather_icon_label = NULL;     /**< 天气图标/描述标签 */

/*---------------------------- 页面2：天气详情组件 ------------------------*/
static lv_obj_t *weather_page = NULL;           /**< 天气详情页面容器 */
static lv_obj_t *weather_temp_label = NULL;     /**< 温度大字显示 */
static lv_obj_t *weather_desc_label = NULL;     /**< 天气描述 */
static lv_obj_t *weather_city_label = NULL;     /**< 城市名称 */
static lv_obj_t *weather_detail_card = NULL;    /**< 详情卡片 */
static lv_obj_t *weather_humidity_label = NULL; /**< 湿度 */
static lv_obj_t *weather_wind_label = NULL;     /**< 风力 */

/*---------------------------- 页面3：设置组件 ----------------------------*/
static lv_obj_t *settings_page = NULL;          /**< 设置页面容器 */
static lv_obj_t *setting_city_label = NULL;     /**< 城市设置显示 */
static lv_obj_t *setting_version_label = NULL;  /**< 版本号显示 */

/*---------------------------- WiFi页面组件 --------------------------------*/
static lv_obj_t *wifi_status_label = NULL;      /**< WiFi连接状态标签 */

/*---------------------------- 启动画面组件 --------------------------------*/
static lv_obj_t *splash_screen = NULL;          /**< 启动画面容器 */
static lv_obj_t *splash_title = NULL;           /**< 启动画面标题 */
static lv_obj_t *splash_subtitle = NULL;        /**< 启动画面副标题 */
static lv_obj_t *splash_bar = NULL;             /**< 进度条前景 */
static lv_obj_t *splash_bar_bg = NULL;          /**< 进度条背景 */
static lv_obj_t *splash_status = NULL;          /**< 状态文本标签 */
static lv_obj_t *logo_circle = NULL;            /**< Logo圆形动画对象 */
static uint8_t splash_progress = 0;              /**< 启动进度值 */

/*---------------------------- 其他 --------------------------------------*/
static uint8_t current_page = 0;                 /**< 当前页面索引 */

/*============================================================================*/
/*                             颜色定义                                       */
/*============================================================================*/

#define COLOR_BG_PRIMARY    lv_color_black()                    /**< 主背景色 */
#define COLOR_BG_CARD       lv_color_make(0x1A, 0x1A, 0x1A)    /**< 卡片背景色 */
#define COLOR_BG_INNER      lv_color_make(0x2E, 0x7D, 0x32)    /**< 室内卡片背景色 */
#define COLOR_BG_OUTDOOR    lv_color_make(0xC6, 0x28, 0x28)    /**< 室外卡片背景色 */
#define COLOR_BG_WEATHER    lv_color_make(0x0D, 0x47, 0xA1)    /**< 天气卡片背景色 */
#define COLOR_TEXT_PRIMARY  lv_color_white()                    /**< 主要文本色 */
#define COLOR_TEXT_SECONDARY lv_color_make(0xAA, 0xAA, 0xAA)   /**< 次要文本色 */
#define COLOR_ACCENT        lv_color_make(0x00, 0xD4, 0xAA)    /**< 强调色 */
#define COLOR_INDICATOR     lv_color_make(0xFF, 0xFF, 0xFF)    /**< 指示器激活色 */
#define COLOR_INDICATOR_DIM lv_color_make(0x55, 0x55, 0x55)    /**< 指示器非激活色 */

/*============================================================================*/
/*                             私有函数声明                                   */
/*============================================================================*/

static void lvgl_lock(void);
static void lvgl_unlock(void);
static void splash_bar_anim_cb(void *var, int32_t v);
static void logo_breathe_anim_cb(void *var, int32_t v);
static void logo_breathe_ready_cb(lv_anim_t *a);
static void fade_in_anim_cb(void *var, int32_t v);
static void tabview_switch_cb(lv_event_t * e);
static void update_page_indicator(uint8_t page);
static void create_status_bar(void);
static void create_time_page(void);
static void create_weather_page(void);
static void create_settings_page(void);
static void create_indicator_bar(void);

/*============================================================================*/
/*                             LVGL线程安全函数                               */
/*============================================================================*/

static void lvgl_lock(void)
{
    if (lcd_mutex != NULL) {
        osMutexAcquire(lcd_mutex, osWaitForever);
    }
}

static void lvgl_unlock(void)
{
    if (lcd_mutex != NULL) {
        osMutexRelease(lcd_mutex);
    }
}

/*============================================================================*/
/*                             动画回调函数                                   */
/*============================================================================*/

static void splash_bar_anim_cb(void *var, int32_t v)
{
    lv_obj_t *bar = (lv_obj_t *)var;
    lv_obj_set_width(bar, (v * 180) / 100);
}

static void logo_breathe_anim_cb(void *var, int32_t v)
{
    lv_obj_t *circle = (lv_obj_t *)var;
    lv_coord_t size = 80 + (v * 10) / 100;
    lv_obj_set_size(circle, size, size);
    lv_obj_align(circle, LV_ALIGN_CENTER, 0, -30);
    lv_color_t c = lv_color_make(0x00, 0xD4 + (v / 5), 0xAA);
    lv_obj_set_style_bg_color(circle, c, LV_PART_MAIN);
}

static void logo_breathe_ready_cb(lv_anim_t *a)
{
    lv_anim_t a2;
    lv_anim_init(&a2);
    lv_anim_set_var(&a2, logo_circle);
    lv_anim_set_exec_cb(&a2, logo_breathe_anim_cb);
    lv_anim_set_values(&a2, 0, 100);
    lv_anim_set_time(&a2, 1500);
    lv_anim_set_playback_time(&a2, 1500);
    lv_anim_set_repeat_count(&a2, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a2);
}

static void fade_in_anim_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
}

/*============================================================================*/
/*                             事件回调函数                                   */
/*============================================================================*/

static void tabview_switch_cb(lv_event_t * e)
{
    lv_obj_t *tv = lv_event_get_target(e);
    uint16_t active = lv_tabview_get_tab_act(tv);
    current_page = active;
    update_page_indicator(active);
}

static void update_page_indicator(uint8_t page)
{
    for (int i = 0; i < 3; i++) {
        if (page_indicator[i] != NULL) {
            if (i == page) {
                lv_obj_set_style_bg_color(page_indicator[i], COLOR_ACCENT, LV_PART_MAIN);
                lv_obj_set_style_opa(page_indicator[i], 255, LV_PART_MAIN);
            } else {
                lv_obj_set_style_bg_color(page_indicator[i], COLOR_INDICATOR_DIM, LV_PART_MAIN);
                lv_obj_set_style_opa(page_indicator[i], 150, LV_PART_MAIN);
            }
        }
    }
}

/*============================================================================*/
/*                             启动画面函数                                   */
/*============================================================================*/

void splash_screen_start(void)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG_PRIMARY, LV_PART_MAIN);

    splash_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(splash_screen, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(splash_screen, 0, 0);
    lv_obj_set_style_bg_color(splash_screen, COLOR_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(splash_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(splash_screen, 0, LV_PART_MAIN);

    logo_circle = lv_obj_create(splash_screen);
    lv_obj_set_size(logo_circle, 80, 80);
    lv_obj_align(logo_circle, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_bg_color(logo_circle, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(logo_circle, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(logo_circle, 40, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(logo_circle, 0, LV_PART_MAIN);

    lv_obj_t *logo_text = lv_label_create(logo_circle);
    lv_label_set_text(logo_text, "W");
    lv_obj_set_style_text_color(logo_text, COLOR_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(logo_text, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_center(logo_text);

    lv_anim_t logo_anim;
    lv_anim_init(&logo_anim);
    lv_anim_set_var(&logo_anim, logo_circle);
    lv_anim_set_exec_cb(&logo_anim, logo_breathe_anim_cb);
    lv_anim_set_values(&logo_anim, 0, 100);
    lv_anim_set_time(&logo_anim, 800);
    lv_anim_set_ready_cb(&logo_anim, logo_breathe_ready_cb);
    lv_anim_start(&logo_anim);

    splash_title = lv_label_create(splash_screen);
    lv_label_set_text(splash_title, "Weather Clock");
    lv_obj_set_style_text_color(splash_title, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(splash_title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_pos(splash_title, 30, 160);
    lv_obj_set_style_opa(splash_title, 0, LV_PART_MAIN);

    splash_subtitle = lv_label_create(splash_screen);
    lv_label_set_text(splash_subtitle, "Smart Desktop Terminal");
    lv_obj_set_style_text_color(splash_subtitle, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(splash_subtitle, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_pos(splash_subtitle, 45, 190);
    lv_obj_set_style_opa(splash_subtitle, 0, LV_PART_MAIN);

    splash_bar_bg = lv_obj_create(splash_screen);
    lv_obj_set_size(splash_bar_bg, 180, 8);
    lv_obj_set_pos(splash_bar_bg, 30, 240);
    lv_obj_set_style_bg_color(splash_bar_bg, lv_color_make(0x2A, 0x2A, 0x2A), LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_bar_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_bar_bg, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(splash_bar_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(splash_bar_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_opa(splash_bar_bg, 0, LV_PART_MAIN);

    splash_bar = lv_obj_create(splash_bar_bg);
    lv_obj_set_size(splash_bar, 0, 8);
    lv_obj_set_pos(splash_bar, 0, 0);
    lv_obj_set_style_bg_color(splash_bar, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(splash_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(splash_bar, 0, LV_PART_MAIN);

    splash_status = lv_label_create(splash_screen);
    lv_label_set_text(splash_status, "Initializing...");
    lv_obj_set_style_text_color(splash_status, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(splash_status, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_pos(splash_status, 30, 260);
    lv_obj_set_style_opa(splash_status, 0, LV_PART_MAIN);

    lv_obj_t *version = lv_label_create(splash_screen);
    lv_label_set_text(version, "v1.2.0");
    lv_obj_set_style_text_color(version, lv_color_make(0x44, 0x44, 0x44), LV_PART_MAIN);
    lv_obj_set_style_text_font(version, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(version, LV_ALIGN_BOTTOM_RIGHT, -15, -10);
    lv_obj_set_style_opa(version, 0, LV_PART_MAIN);

    lv_anim_t title_anim;
    lv_anim_init(&title_anim);
    lv_anim_set_var(&title_anim, splash_title);
    lv_anim_set_exec_cb(&title_anim, fade_in_anim_cb);
    lv_anim_set_values(&title_anim, 0, 255);
    lv_anim_set_time(&title_anim, 600);
    lv_anim_set_delay(&title_anim, 300);
    lv_anim_start(&title_anim);

    lv_anim_t subtitle_anim;
    lv_anim_init(&subtitle_anim);
    lv_anim_set_var(&subtitle_anim, splash_subtitle);
    lv_anim_set_exec_cb(&subtitle_anim, fade_in_anim_cb);
    lv_anim_set_values(&subtitle_anim, 0, 255);
    lv_anim_set_time(&subtitle_anim, 600);
    lv_anim_set_delay(&subtitle_anim, 600);
    lv_anim_start(&subtitle_anim);

    lv_anim_t bar_bg_anim;
    lv_anim_init(&bar_bg_anim);
    lv_anim_set_var(&bar_bg_anim, splash_bar_bg);
    lv_anim_set_exec_cb(&bar_bg_anim, fade_in_anim_cb);
    lv_anim_set_values(&bar_bg_anim, 0, 255);
    lv_anim_set_time(&bar_bg_anim, 400);
    lv_anim_set_delay(&bar_bg_anim, 900);
    lv_anim_start(&bar_bg_anim);

    lv_anim_t status_anim;
    lv_anim_init(&status_anim);
    lv_anim_set_var(&status_anim, splash_status);
    lv_anim_set_exec_cb(&status_anim, fade_in_anim_cb);
    lv_anim_set_values(&status_anim, 0, 255);
    lv_anim_set_time(&status_anim, 400);
    lv_anim_set_delay(&status_anim, 1100);
    lv_anim_start(&status_anim);

    lv_anim_t version_anim;
    lv_anim_init(&version_anim);
    lv_anim_set_var(&version_anim, version);
    lv_anim_set_exec_cb(&version_anim, fade_in_anim_cb);
    lv_anim_set_values(&version_anim, 0, 255);
    lv_anim_set_time(&version_anim, 400);
    lv_anim_set_delay(&version_anim, 1300);
    lv_anim_start(&version_anim);

    lv_anim_t bar_anim;
    lv_anim_init(&bar_anim);
    lv_anim_set_var(&bar_anim, splash_bar);
    lv_anim_set_exec_cb(&bar_anim, splash_bar_anim_cb);
    lv_anim_set_values(&bar_anim, 0, 100);
    lv_anim_set_time(&bar_anim, 2000);
    lv_anim_set_delay(&bar_anim, 1500);
    lv_anim_start(&bar_anim);

    lvgl_unlock();
}

void splash_set_progress(uint8_t progress, const char *status)
{
    lvgl_lock();
    
    splash_progress = progress;
    
    if (splash_bar && progress <= 100) {
        lv_obj_set_width(splash_bar, (progress * 180) / 100);
    }
    
    if (splash_status && status) {
        lv_label_set_text(splash_status, status);
    }
    
    lvgl_unlock();
}

void splash_screen_end(void)
{
    lvgl_lock();
    
    if (splash_screen) {
        lv_anim_t fade_out;
        lv_anim_init(&fade_out);
        lv_anim_set_var(&fade_out, splash_screen);
        lv_anim_set_exec_cb(&fade_out, fade_in_anim_cb);
        lv_anim_set_values(&fade_out, 255, 0);
        lv_anim_set_time(&fade_out, 500);
        lv_anim_start(&fade_out);

        lv_obj_del_delayed(splash_screen, 600);
        splash_screen = NULL;
        splash_title = NULL;
        splash_subtitle = NULL;
        splash_bar = NULL;
        splash_bar_bg = NULL;
        splash_status = NULL;
        logo_circle = NULL;
    }
    
    lvgl_unlock();
}

void welcome_page_display(void)
{
    splash_screen_start();
}

/*============================================================================*/
/*                             WiFi页面函数                                   */
/*============================================================================*/

void wifi_page_display(void)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG_PRIMARY, LV_PART_MAIN);

    lv_obj_t *icon = lv_label_create(lv_scr_act());
    lv_label_set_text(icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(icon, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -60);
    lv_obj_set_style_opa(icon, 0, LV_PART_MAIN);

    lv_obj_t *ssid_label = lv_label_create(lv_scr_act());
    lv_label_set_text(ssid_label, WIFI_SSID);
    lv_obj_set_style_text_color(ssid_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_opa(ssid_label, 0, LV_PART_MAIN);

    wifi_status_label = lv_label_create(lv_scr_act());
    lv_label_set_text(wifi_status_label, "Connecting...");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_make(0xFF, 0xAA, 0x00), LV_PART_MAIN);
    lv_obj_set_style_text_font(wifi_status_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(wifi_status_label, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_opa(wifi_status_label, 0, LV_PART_MAIN);

    lv_obj_t *spinner = lv_spinner_create(lv_scr_act(), 1000, 60);
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 100);
    lv_obj_set_style_arc_color(spinner, lv_color_make(0x33, 0x33, 0x33), LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(spinner, 4, LV_PART_INDICATOR);
    lv_obj_set_style_opa(spinner, 0, LV_PART_MAIN);

    lv_anim_t anims[4];
    lv_obj_t *objs[4] = {icon, ssid_label, wifi_status_label, spinner};
    for (int i = 0; i < 4; i++) {
        lv_anim_init(&anims[i]);
        lv_anim_set_var(&anims[i], objs[i]);
        lv_anim_set_exec_cb(&anims[i], fade_in_anim_cb);
        lv_anim_set_values(&anims[i], 0, 255);
        lv_anim_set_time(&anims[i], 400);
        lv_anim_set_delay(&anims[i], i * 150);
        lv_anim_start(&anims[i]);
    }

    lvgl_unlock();
}

void wifi_page_set_status(const char *status, bool success)
{
    lvgl_lock();
    if (wifi_status_label) {
        lv_label_set_text(wifi_status_label, status);
        if (success) {
            lv_obj_set_style_text_color(wifi_status_label, lv_color_make(0x00, 0xFF, 0x00), LV_PART_MAIN);
        } else {
            lv_obj_set_style_text_color(wifi_status_label, lv_color_make(0xFF, 0x44, 0x44), LV_PART_MAIN);
        }
    }
    lvgl_unlock();
}

/*============================================================================*/
/*                             错误页面函数                                   */
/*============================================================================*/

void error_page_display(const char *msg)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG_PRIMARY, LV_PART_MAIN);

    lv_obj_t *icon = lv_label_create(lv_scr_act());
    lv_label_set_text(icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(icon, lv_color_make(0xFF, 0x44, 0x44), LV_PART_MAIN);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_opa(icon, 0, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, msg);
    lv_obj_set_style_text_color(label, lv_color_make(0xFF, 0xAA, 0x00), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_width(label, 200);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_opa(label, 0, LV_PART_MAIN);

    lv_anim_t icon_anim;
    lv_anim_init(&icon_anim);
    lv_anim_set_var(&icon_anim, icon);
    lv_anim_set_exec_cb(&icon_anim, fade_in_anim_cb);
    lv_anim_set_values(&icon_anim, 0, 255);
    lv_anim_set_time(&icon_anim, 500);
    lv_anim_start(&icon_anim);

    lv_anim_t label_anim;
    lv_anim_init(&label_anim);
    lv_anim_set_var(&label_anim, label);
    lv_anim_set_exec_cb(&label_anim, fade_in_anim_cb);
    lv_anim_set_values(&label_anim, 0, 255);
    lv_anim_set_time(&label_anim, 500);
    lv_anim_set_delay(&label_anim, 200);
    lv_anim_start(&label_anim);

    lvgl_unlock();
}

/*============================================================================*/
/*                             主页面函数 - 多页面滑动                         */
/*============================================================================*/

void main_page_display(void)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG_PRIMARY, LV_PART_MAIN);

    create_status_bar();

    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_LEFT, 0);
    lv_obj_set_size(tabview, SCREEN_WIDTH, CONTENT_HEIGHT);
    lv_obj_set_pos(tabview, 0, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(tabview, COLOR_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(tabview, 0, LV_PART_MAIN);
    lv_obj_clear_flag(tabview, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_add_event_cb(tabview, tabview_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

    time_page = lv_tabview_add_tab(tabview, "Time");
    weather_page = lv_tabview_add_tab(tabview, "Weather");
    settings_page = lv_tabview_add_tab(tabview, "Settings");
    
    lv_obj_set_style_bg_color(time_page, COLOR_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_color(weather_page, COLOR_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_bg_color(settings_page, COLOR_BG_PRIMARY, LV_PART_MAIN);
    
    lv_obj_clear_flag(time_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(weather_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(settings_page, LV_OBJ_FLAG_SCROLLABLE);

    create_time_page();
    create_weather_page();
    create_settings_page();
    create_indicator_bar();

    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_add_flag(tab_btns, LV_OBJ_FLAG_HIDDEN);

    current_page = 0;
    lvgl_unlock();
}

static void create_status_bar(void)
{
    status_bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(status_bar, SCREEN_WIDTH, STATUS_BAR_HEIGHT);
    lv_obj_set_pos(status_bar, 0, 0);
    lv_obj_set_style_bg_color(status_bar, lv_color_make(0x15, 0x15, 0x15), LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(status_bar, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(status_bar, 10, LV_PART_MAIN);

    status_wifi_icon = lv_label_create(status_bar);
    lv_label_set_text(status_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(status_wifi_icon, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(status_wifi_icon, &lv_font_montserrat_14, LV_PART_MAIN);

    status_wifi_label = lv_label_create(status_bar);
    lv_label_set_text(status_wifi_label, WIFI_SSID);
    lv_obj_set_style_text_color(status_wifi_label, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(status_wifi_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_pad_left(status_wifi_label, 5, LV_PART_MAIN);
}

static void create_time_page(void)
{
    lv_obj_set_flex_flow(time_page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(time_page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(time_page, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_left(time_page, 15, LV_PART_MAIN);
    lv_obj_set_style_pad_right(time_page, 15, LV_PART_MAIN);
    lv_obj_set_style_pad_row(time_page, 15, LV_PART_MAIN);

    lv_obj_t *time_container = lv_obj_create(time_page);
    lv_obj_set_size(time_container, SCREEN_WIDTH - 30, 90);
    lv_obj_set_style_bg_color(time_container, COLOR_BG_CARD, LV_PART_MAIN);
    lv_obj_set_style_border_width(time_container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(time_container, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(time_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(time_container, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(time_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(time_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    time_label = lv_label_create(time_container);
    lv_label_set_text(time_label, "--:--:--");
    lv_obj_set_style_text_color(time_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_32, LV_PART_MAIN);

    date_label = lv_label_create(time_container);
    lv_label_set_text(date_label, "----/--/--");
    lv_obj_set_style_text_color(date_label, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_14, LV_PART_MAIN);

    indoor_card = lv_obj_create(time_page);
    lv_obj_set_size(indoor_card, SCREEN_WIDTH - 30, 70);
    lv_obj_set_style_bg_color(indoor_card, COLOR_BG_INNER, LV_PART_MAIN);
    lv_obj_set_style_border_width(indoor_card, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(indoor_card, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(indoor_card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(indoor_card, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(indoor_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(indoor_card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *indoor_left = lv_obj_create(indoor_card);
    lv_obj_set_size(indoor_left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(indoor_left, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(indoor_left, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(indoor_left, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(indoor_left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(indoor_left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *indoor_title = lv_label_create(indoor_left);
    lv_label_set_text(indoor_title, "Indoor");
    lv_obj_set_style_text_color(indoor_title, lv_color_make(0xCC, 0xFF, 0xCC), LV_PART_MAIN);
    lv_obj_set_style_text_font(indoor_title, &lv_font_montserrat_14, LV_PART_MAIN);

    indoor_temp_label = lv_label_create(indoor_left);
    lv_label_set_text(indoor_temp_label, "-- C");
    lv_obj_set_style_text_color(indoor_temp_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(indoor_temp_label, &lv_font_montserrat_24, LV_PART_MAIN);

    lv_obj_t *indoor_right = lv_obj_create(indoor_card);
    lv_obj_set_size(indoor_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(indoor_right, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(indoor_right, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(indoor_right, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(indoor_right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(indoor_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *humid_title = lv_label_create(indoor_right);
    lv_label_set_text(humid_title, "Humidity");
    lv_obj_set_style_text_color(humid_title, lv_color_make(0xCC, 0xFF, 0xCC), LV_PART_MAIN);
    lv_obj_set_style_text_font(humid_title, &lv_font_montserrat_14, LV_PART_MAIN);

    indoor_humidity_label = lv_label_create(indoor_right);
    lv_label_set_text(indoor_humidity_label, "-- %");
    lv_obj_set_style_text_color(indoor_humidity_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(indoor_humidity_label, &lv_font_montserrat_20, LV_PART_MAIN);

    outdoor_card = lv_obj_create(time_page);
    lv_obj_set_size(outdoor_card, SCREEN_WIDTH - 30, 70);
    lv_obj_set_style_bg_color(outdoor_card, COLOR_BG_OUTDOOR, LV_PART_MAIN);
    lv_obj_set_style_border_width(outdoor_card, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(outdoor_card, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(outdoor_card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(outdoor_card, 10, LV_PART_MAIN);
    lv_obj_set_flex_flow(outdoor_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(outdoor_card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *outdoor_left = lv_obj_create(outdoor_card);
    lv_obj_set_size(outdoor_left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(outdoor_left, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(outdoor_left, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(outdoor_left, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(outdoor_left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(outdoor_left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *outdoor_title = lv_label_create(outdoor_left);
    lv_label_set_text(outdoor_title, "Outdoor");
    lv_obj_set_style_text_color(outdoor_title, lv_color_make(0xFF, 0xCC, 0xCC), LV_PART_MAIN);
    lv_obj_set_style_text_font(outdoor_title, &lv_font_montserrat_14, LV_PART_MAIN);

    outdoor_temp_label = lv_label_create(outdoor_left);
    lv_label_set_text(outdoor_temp_label, "-- C");
    lv_obj_set_style_text_color(outdoor_temp_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(outdoor_temp_label, &lv_font_montserrat_24, LV_PART_MAIN);

    lv_obj_t *outdoor_right = lv_obj_create(outdoor_card);
    lv_obj_set_size(outdoor_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(outdoor_right, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(outdoor_right, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(outdoor_right, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(outdoor_right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(outdoor_right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *weather_title = lv_label_create(outdoor_right);
    lv_label_set_text(weather_title, "Weather");
    lv_obj_set_style_text_color(weather_title, lv_color_make(0xFF, 0xCC, 0xCC), LV_PART_MAIN);
    lv_obj_set_style_text_font(weather_title, &lv_font_montserrat_14, LV_PART_MAIN);

    weather_icon_label = lv_label_create(outdoor_right);
    lv_label_set_text(weather_icon_label, "--");
    lv_obj_set_style_text_color(weather_icon_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(weather_icon_label, &lv_font_montserrat_16, LV_PART_MAIN);
}

static void create_weather_page(void)
{
    lv_obj_set_flex_flow(weather_page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(weather_page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(weather_page, 30, LV_PART_MAIN);
    lv_obj_set_style_pad_left(weather_page, 15, LV_PART_MAIN);
    lv_obj_set_style_pad_right(weather_page, 15, LV_PART_MAIN);
    lv_obj_set_style_pad_row(weather_page, 20, LV_PART_MAIN);

    weather_city_label = lv_label_create(weather_page);
    lv_label_set_text(weather_city_label, "Hengyang");
    lv_obj_set_style_text_color(weather_city_label, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(weather_city_label, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *temp_container = lv_obj_create(weather_page);
    lv_obj_set_size(temp_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(temp_container, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(temp_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(temp_container, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(temp_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(temp_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);

    weather_temp_label = lv_label_create(temp_container);
    lv_label_set_text(weather_temp_label, "--");
    lv_obj_set_style_text_color(weather_temp_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(weather_temp_label, &lv_font_montserrat_32, LV_PART_MAIN);

    lv_obj_t *temp_unit = lv_label_create(temp_container);
    lv_label_set_text(temp_unit, "C");
    lv_obj_set_style_text_color(temp_unit, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(temp_unit, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(temp_unit, 5, LV_PART_MAIN);

    weather_desc_label = lv_label_create(weather_page);
    lv_label_set_text(weather_desc_label, "Loading...");
    lv_obj_set_style_text_color(weather_desc_label, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(weather_desc_label, &lv_font_montserrat_16, LV_PART_MAIN);

    weather_detail_card = lv_obj_create(weather_page);
    lv_obj_set_size(weather_detail_card, SCREEN_WIDTH - 30, 80);
    lv_obj_set_style_bg_color(weather_detail_card, COLOR_BG_CARD, LV_PART_MAIN);
    lv_obj_set_style_border_width(weather_detail_card, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(weather_detail_card, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(weather_detail_card, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(weather_detail_card, 15, LV_PART_MAIN);
    lv_obj_set_flex_flow(weather_detail_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(weather_detail_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(weather_detail_card, 8, LV_PART_MAIN);

    weather_humidity_label = lv_label_create(weather_detail_card);
    lv_label_set_text(weather_humidity_label, "Humidity: --%");
    lv_obj_set_style_text_color(weather_humidity_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(weather_humidity_label, &lv_font_montserrat_14, LV_PART_MAIN);

    weather_wind_label = lv_label_create(weather_detail_card);
    lv_label_set_text(weather_wind_label, "Wind: -- km/h");
    lv_obj_set_style_text_color(weather_wind_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(weather_wind_label, &lv_font_montserrat_14, LV_PART_MAIN);
}

static void create_settings_page(void)
{
    lv_obj_set_flex_flow(settings_page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(settings_page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_top(settings_page, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_left(settings_page, 15, LV_PART_MAIN);
    lv_obj_set_style_pad_right(settings_page, 15, LV_PART_MAIN);
    lv_obj_set_style_pad_row(settings_page, 10, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(settings_page);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, LV_PART_MAIN);

    lv_obj_t *divider1 = lv_obj_create(settings_page);
    lv_obj_set_size(divider1, SCREEN_WIDTH - 30, 1);
    lv_obj_set_style_bg_color(divider1, lv_color_make(0x33, 0x33, 0x33), LV_PART_MAIN);
    lv_obj_set_style_border_width(divider1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(divider1, 0, LV_PART_MAIN);

    lv_obj_t *city_row = lv_obj_create(settings_page);
    lv_obj_set_size(city_row, SCREEN_WIDTH - 30, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(city_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(city_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(city_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(city_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(city_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *city_label = lv_label_create(city_row);
    lv_label_set_text(city_label, "City");
    lv_obj_set_style_text_color(city_label, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(city_label, &lv_font_montserrat_14, LV_PART_MAIN);

    setting_city_label = lv_label_create(city_row);
    lv_label_set_text(setting_city_label, "Hengyang");
    lv_obj_set_style_text_color(setting_city_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(setting_city_label, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *refresh_row = lv_obj_create(settings_page);
    lv_obj_set_size(refresh_row, SCREEN_WIDTH - 30, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(refresh_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(refresh_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(refresh_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(refresh_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(refresh_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *refresh_label = lv_label_create(refresh_row);
    lv_label_set_text(refresh_label, "Refresh Interval");
    lv_obj_set_style_text_color(refresh_label, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(refresh_label, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *refresh_val = lv_label_create(refresh_row);
    lv_label_set_text(refresh_val, "1 min");
    lv_obj_set_style_text_color(refresh_val, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(refresh_val, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *divider2 = lv_obj_create(settings_page);
    lv_obj_set_size(divider2, SCREEN_WIDTH - 30, 1);
    lv_obj_set_style_bg_color(divider2, lv_color_make(0x33, 0x33, 0x33), LV_PART_MAIN);
    lv_obj_set_style_border_width(divider2, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(divider2, 0, LV_PART_MAIN);

    lv_obj_t *about_title = lv_label_create(settings_page);
    lv_label_set_text(about_title, "About");
    lv_obj_set_style_text_color(about_title, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(about_title, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *device_row = lv_obj_create(settings_page);
    lv_obj_set_size(device_row, SCREEN_WIDTH - 30, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(device_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(device_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(device_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(device_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(device_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *device_label = lv_label_create(device_row);
    lv_label_set_text(device_label, "Device");
    lv_obj_set_style_text_color(device_label, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(device_label, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *device_val = lv_label_create(device_row);
    lv_label_set_text(device_val, "STM32F407");
    lv_obj_set_style_text_color(device_val, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(device_val, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *wifi_row = lv_obj_create(settings_page);
    lv_obj_set_size(wifi_row, SCREEN_WIDTH - 30, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(wifi_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifi_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(wifi_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifi_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *wifi_label = lv_label_create(wifi_row);
    lv_label_set_text(wifi_label, "WiFi Module");
    lv_obj_set_style_text_color(wifi_label, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *wifi_val = lv_label_create(wifi_row);
    lv_label_set_text(wifi_val, "ESP8266");
    lv_obj_set_style_text_color(wifi_val, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(wifi_val, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *version_row = lv_obj_create(settings_page);
    lv_obj_set_size(version_row, SCREEN_WIDTH - 30, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(version_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(version_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(version_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(version_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(version_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *version_label = lv_label_create(version_row);
    lv_label_set_text(version_label, "Version");
    lv_obj_set_style_text_color(version_label, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(version_label, &lv_font_montserrat_14, LV_PART_MAIN);

    setting_version_label = lv_label_create(version_row);
    lv_label_set_text(setting_version_label, "v1.2.0");
    lv_obj_set_style_text_color(setting_version_label, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(setting_version_label, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_t *hint = lv_label_create(settings_page);
    lv_label_set_text(hint, LV_SYMBOL_LEFT " Slide to switch " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(hint, lv_color_make(0x44, 0x44, 0x44), LV_PART_MAIN);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

static void create_indicator_bar(void)
{
    lv_obj_t *indic = lv_obj_create(lv_scr_act());
    lv_obj_set_size(indic, 60, 10);
    lv_obj_set_pos(indic, (SCREEN_WIDTH - 60) / 2, SCREEN_HEIGHT - INDICATOR_HEIGHT / 2 - 5);
    lv_obj_set_style_bg_color(indic, COLOR_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(indic, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(indic, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_all(indic, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(indic, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(indic, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    for (int i = 0; i < 3; i++) {
        page_indicator[i] = lv_obj_create(indic);
        lv_obj_set_size(page_indicator[i], 8, 8);
        lv_obj_set_style_bg_color(page_indicator[i], i == 0 ? COLOR_ACCENT : COLOR_INDICATOR_DIM, LV_PART_MAIN);
        lv_obj_set_style_border_width(page_indicator[i], 0, LV_PART_MAIN);
        lv_obj_set_style_radius(page_indicator[i], 4, LV_PART_MAIN);
        lv_obj_set_style_opa(page_indicator[i], i == 0 ? 255 : 150, LV_PART_MAIN);
    }
}

/*============================================================================*/
/*                             数据更新函数                                   */
/*============================================================================*/

void main_page_redraw_wifi_ssid(const char *ssid)
{
    lvgl_lock();
    if (status_wifi_label) {
        lv_label_set_text(status_wifi_label, ssid);
    }
    lvgl_unlock();
}

void main_page_redraw_time(rtc_date_time_t *time)
{
    lvgl_lock();
    if (time_label) {
        char str[16];
        snprintf(str, sizeof(str), "%02u:%02u:%02u", time->hour, time->minute, time->second);
        lv_label_set_text(time_label, str);
    }
    lvgl_unlock();
}

void main_page_redraw_date(rtc_date_time_t *date)
{
    lvgl_lock();
    if (date_label) {
        const char *week_str[] = {"", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        char str[32];
        if (date->weekday >= 1 && date->weekday <= 7) {
            snprintf(str, sizeof(str), "%04u/%02u/%02u %s", date->year, date->month, date->day, week_str[date->weekday]);
        } else {
            snprintf(str, sizeof(str), "%04u/%02u/%02u", date->year, date->month, date->day);
        }
        lv_label_set_text(date_label, str);
    }
    lvgl_unlock();
}

void main_page_redraw_inner_temperature(float temperature)
{
    lvgl_lock();
    if (indoor_temp_label) {
        char str[16];
        int temp_int = (int)temperature;
        if (temp_int >= 0 && temp_int <= 100) {
            snprintf(str, sizeof(str), "%d C", temp_int);
        } else {
            snprintf(str, sizeof(str), "-- C");
        }
        lv_label_set_text(indoor_temp_label, str);
    }
    lvgl_unlock();
}

void main_page_redraw_inner_humidity(float humidity)
{
    lvgl_lock();
    if (indoor_humidity_label) {
        char str[16];
        int humid_int = (int)humidity;
        if (humid_int >= 0 && humid_int <= 100) {
            snprintf(str, sizeof(str), "%d %%", humid_int);
        } else {
            snprintf(str, sizeof(str), "-- %%");
        }
        lv_label_set_text(indoor_humidity_label, str);
    }
    lvgl_unlock();
}

void main_page_redraw_outdoor_city(const char *city)
{
    lvgl_lock();
    if (setting_city_label) {
        lv_label_set_text(setting_city_label, city);
    }
    if (weather_city_label) {
        lv_label_set_text(weather_city_label, city);
    }
    lvgl_unlock();
}

void main_page_redraw_outdoor_temperature(float temperature)
{
    lvgl_lock();
    if (outdoor_temp_label) {
        char str[16];
        if (temperature > -10.0f && temperature <= 100.0f) {
            snprintf(str, sizeof(str), "%d C", (int)temperature);
        } else {
            snprintf(str, sizeof(str), "-- C");
        }
        lv_label_set_text(outdoor_temp_label, str);
    }
    if (weather_temp_label) {
        char str[16];
        if (temperature > -10.0f && temperature <= 100.0f) {
            snprintf(str, sizeof(str), "%d", (int)temperature);
        } else {
            snprintf(str, sizeof(str), "--");
        }
        lv_label_set_text(weather_temp_label, str);
    }
    lvgl_unlock();
}

void main_page_redraw_outdoor_weather_icon(const int code)
{
    lvgl_lock();
    if (weather_icon_label) {
        const char *icon;
        if (code == 0 || code == 2 || code == 38) {
            icon = "* Sunny";
        } else if (code == 1 || code == 3) {
            icon = "* Clear";
        } else if (code == 4 || code == 9 || code == 30) {
            icon = "~ Cloudy";
        } else if (code == 5 || code == 6 || code == 7 || code == 8) {
            icon = "~ Partly";
        } else if (code >= 10 && code <= 19) {
            icon = "o Rain";
        } else if (code == 11 || code == 12) {
            icon = LV_SYMBOL_CHARGE " Thunder";
        } else if (code >= 20 && code <= 25) {
            icon = "* Snow";
        } else {
            icon = "--";
        }
        lv_label_set_text(weather_icon_label, icon);
        
        if (weather_desc_label) {
            const char *desc = icon;
            while (*desc == '*' || *desc == '~' || *desc == 'o' || *desc == ' ') {
                desc++;
            }
            lv_label_set_text(weather_desc_label, desc);
        }
    }
    lvgl_unlock();
}

void main_page_redraw_outdoor_humidity(int humidity)
{
    lvgl_lock();
    if (weather_humidity_label) {
        char str[32];
        if (humidity >= 0 && humidity <= 100) {
            snprintf(str, sizeof(str), "Humidity: %d%%", humidity);
        } else {
            snprintf(str, sizeof(str), "Humidity: --%%");
        }
        lv_label_set_text(weather_humidity_label, str);
    }
    lvgl_unlock();
}

void main_page_redraw_outdoor_wind(int wind_speed, const char *wind_direction)
{
    lvgl_lock();
    if (weather_wind_label) {
        char str[32];
        if (wind_speed >= 0 && wind_direction && strlen(wind_direction) > 0) {
            snprintf(str, sizeof(str), "Wind: %s %d km/h", wind_direction, wind_speed);
        } else if (wind_speed >= 0) {
            snprintf(str, sizeof(str), "Wind: %d km/h", wind_speed);
        } else {
            snprintf(str, sizeof(str), "Wind: -- km/h");
        }
        lv_label_set_text(weather_wind_label, str);
    }
    lvgl_unlock();
}
