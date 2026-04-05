/**
 * @file lvgl_pages.c
 * @brief LVGL页面显示模块 - 桌面智能终端UI实现
 * * 此文件实现了基于LVGL图形库的智能终端用户界面，主要包含以下页面功能：
 * - 欢迎页面(Splash Screen)：显示Logo、产品名称、进度条及状态信息
 * - WiFi配网页面：显示WiFi连接状态和动画
 * - 错误提示页：显示系统或网络警告信息
 * - 主界面：支持滑动切换的多页面系统
 * - 页面1：时间与环境展示页（室内外温湿度、天气）
 * - 页面2：天气详情页
 * - 页面3：系统设置与设备信息页
 * * 屏幕布局(240x320屏幕)：
 * +------------------+
 * |  状态栏 (24px)   |  WiFi图标/SSID
 * +------------------+
 * |                  |
 * |  主内容区域      |
 * |  (276px)         |
 * |                  |
 * +------------------+
 * | 底部指示器(20px) |
 * +------------------+
 * * @author Smart Weather Clock Team
 * @version 1.2.2
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
#include "imag.h"

/*============================================================================*/
/* 宏定义与常量                                   */
/*============================================================================*/

#define STATUS_BAR_HEIGHT   24      /**< 状态栏高度 */
#define INDICATOR_HEIGHT    20      /**< 底部页面指示器高度 */
#define CONTENT_HEIGHT      276     /**< 内容区高度 (320 - 24 - 20) */
#define SCREEN_WIDTH        240     /**< 屏幕宽度 */
#define SCREEN_HEIGHT       320     /**< 屏幕高度 */

/*============================================================================*/
/* 外部变量声明                                   */
/*============================================================================*/

/* 引入在其他地方(通常是FreeRTOS任务初始化时)创建的LVGL互斥锁 */
extern osMutexId_t lcd_mutex;

/*============================================================================*/
/* 静态全局变量                                   */
/*============================================================================*/

/*---------------------------- 全局状态栏对象 ----------------------------------*/
static lv_obj_t *status_bar = NULL;             /**< 状态栏容器 */
static lv_obj_t *status_wifi_icon = NULL;       /**< WiFi信号图标文本对象 */
static lv_obj_t *status_wifi_label = NULL;      /**< WiFi SSID文本标签 */

/*---------------------------- 主视图与导航 --------------------------------*/
static lv_obj_t *tabview = NULL;                /**< 滑动标签页视图控制器 */
static lv_obj_t *page_indicator[3] = {NULL};    /**< 底部页面位置指示器(小圆点) */

/*---------------------------- 页面1：时间与环境展示页 ------------------------*/
static lv_obj_t *time_page = NULL;              /**< 时间显示页容器 */
static lv_obj_t *time_label = NULL;             /**< 时分秒显示标签 */
static lv_obj_t *date_label = NULL;             /**< 年月日星期显示标签 */
static lv_obj_t *indoor_card = NULL;            /**< 室内环境数据卡片背景 */
static lv_obj_t *indoor_temp_label = NULL;      /**< 室内温度标签 */
static lv_obj_t *indoor_humidity_label = NULL;  /**< 室内湿度标签 */
static lv_obj_t *outdoor_card = NULL;           /**< 室外环境数据卡片背景 */
static lv_obj_t *outdoor_temp_label = NULL;     /**< 室外温度标签 */
static lv_obj_t *weather_icon_label = NULL;     /**< 天气图标/简述标签 */

/*---------------------------- 页面2：天气详情展示页 ------------------------*/
static lv_obj_t *weather_page = NULL;           /**< 天气详情页容器 */
static lv_obj_t *weather_temp_label = NULL;     /**< 大字号温度显示 */
static lv_obj_t *weather_desc_label = NULL;     /**< 天气详细描述(如: 晴转多云) */
static lv_obj_t *weather_city_label = NULL;     /**< 当前定位城市 */
static lv_obj_t *weather_detail_card = NULL;    /**< 详细数据卡片背景 */
static lv_obj_t *weather_humidity_label = NULL; /**< 室外湿度详情 */
static lv_obj_t *weather_wind_label = NULL;     /**< 风向风力详情 */

/*---------------------------- 页面3：系统设置与信息 ----------------------------*/
static lv_obj_t *settings_page = NULL;          /**< 设置页容器 */
static lv_obj_t *setting_city_label = NULL;     /**< 设置页定位城市显示 */
static lv_obj_t *setting_version_label = NULL;  /**< 固件版本号显示 */

/*---------------------------- WiFi配网页对象 --------------------------------*/
static lv_obj_t *wifi_status_label = NULL;      /**< WiFi连接状态文本 */

/*---------------------------- 欢迎/启动页对象 --------------------------------*/
static lv_obj_t *splash_screen = NULL;          /**< 欢迎页根容器 */
static lv_obj_t *splash_title = NULL;           /**< 主标题文本 */
static lv_obj_t *splash_bar = NULL;             /**< 进度条前景(动态增长部分) */
static lv_obj_t *splash_bar_bg = NULL;          /**< 进度条背景槽 */
static lv_obj_t *splash_status = NULL;          /**< 启动阶段状态文本 */
static lv_obj_t *splash_logo_img = NULL;        /**< Logo图片对象 */
static lv_obj_t *splash_progress_label = NULL;  /**< 【新增】跟随进度条的百分比文字 */
static lv_img_dsc_t logo_img_dsc;               /**< LVGL图片描述符结构体 */
static uint8_t splash_progress = 0;              /**< 当前进度值(0-100) */

/*---------------------------- 系统状态记录 --------------------------------------*/
static uint8_t current_page = 0;                 /**< 当前所在的Tab页面索引(0, 1, 2) */

/*============================================================================*/
/* UI 颜色主题定义                                */
/*============================================================================*/

#define COLOR_BG_PRIMARY    lv_color_black()                    /**< 主背景色 (纯黑) */
#define COLOR_BG_CARD       lv_color_make(0x1A, 0x1A, 0x1A)    /**< 普通卡片背景色 (深灰) */
#define COLOR_BG_INNER      lv_color_make(0x2E, 0x7D, 0x32)    /**< 室内环境卡片背景 (墨绿) */
#define COLOR_BG_OUTDOOR    lv_color_make(0xC6, 0x28, 0x28)    /**< 室外环境卡片背景 (深红) */
#define COLOR_BG_WEATHER    lv_color_make(0x0D, 0x47, 0xA1)    /**< 天气专属卡片背景 (深蓝) */
#define COLOR_TEXT_PRIMARY  lv_color_white()                    /**< 主要文本颜色 (纯白) */
#define COLOR_TEXT_SECONDARY lv_color_make(0xAA, 0xAA, 0xAA)   /**< 次要文本颜色 (浅灰) */
#define COLOR_ACCENT        lv_color_make(0x00, 0xD4, 0xAA)    /**< 强调色/主题色 (青绿色) */
#define COLOR_INDICATOR     lv_color_make(0xFF, 0xFF, 0xFF)    /**< 激活状态的指示器颜色 */
#define COLOR_INDICATOR_DIM lv_color_make(0x55, 0x55, 0x55)    /**< 未激活状态的指示器颜色 */

/*============================================================================*/
/* 私有函数声明                                   */
/*============================================================================*/

static void lvgl_lock(void);
static void lvgl_unlock(void);
static void slide_y_anim_cb(void *var, int32_t v);
static void fade_in_anim_cb(void *var, int32_t v);
static void splash_bar_anim_cb(void *var, int32_t v);
static void tabview_switch_cb(lv_event_t * e);
static void update_page_indicator(uint8_t page);
static void create_status_bar(void);
static void create_time_page(void);
static void create_weather_page(void);
static void create_settings_page(void);
static void create_indicator_bar(void);

/*============================================================================*/
/* LVGL线程锁封装                                 */
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
/* 动画回调函数实现                               */
/*============================================================================*/

static void slide_y_anim_cb(void *var, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)var, v);
}

static void fade_in_anim_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
}

/**
 * @brief 进度条宽度变化回调，同时驱动百分比文字跟随
 */
static void splash_bar_anim_cb(void *var, int32_t v)
{
    lv_obj_t *bar = (lv_obj_t *)var;
    lv_obj_set_width(bar, v);

    // 【新增核心】让百分比文字跟随进度条的末端移动
    if (splash_progress_label != NULL) {
        int percent = (v * 100) / 160; // 160是背景槽的最大宽度
        lv_label_set_text_fmt(splash_progress_label, "%d%%", percent);
        
        // 动态对齐：始终锚定在进度条当前宽度的右上角外部，向右偏10像素，向上偏8像素
        lv_obj_align_to(splash_progress_label, bar, LV_ALIGN_OUT_TOP_RIGHT, 10, -8);
    }
}

/*============================================================================*/
/* UI事件回调函数                                 */
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
/* 欢迎/启动页实现                                */
/*============================================================================*/

void splash_screen_start(void)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG_PRIMARY, LV_PART_MAIN);

    /* 1. 创建全屏背景容器 */
    splash_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(splash_screen, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(splash_screen, COLOR_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_screen, 0, LV_PART_MAIN);
    lv_obj_clear_flag(splash_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* 2. 挂载并配置 Logo 图片 */
    logo_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    logo_img_dsc.header.always_zero = 0;
    logo_img_dsc.header.reserved = 0;
    logo_img_dsc.header.w = img_chengpingan.width;
    logo_img_dsc.header.h = img_chengpingan.height;
    logo_img_dsc.data_size = img_chengpingan.width * img_chengpingan.height * 2;
    logo_img_dsc.data = img_chengpingan.data;

    splash_logo_img = lv_img_create(splash_screen);
    lv_img_set_src(splash_logo_img, &logo_img_dsc);
    
    // 锚定顶部居中
    lv_obj_align(splash_logo_img, LV_ALIGN_TOP_MID, 0, 0); 
    lv_obj_set_style_opa(splash_logo_img, 0, LV_PART_MAIN);

    /* --- Logo 动画 --- */
    lv_anim_t a_logo_y, a_logo_opa;
    lv_anim_init(&a_logo_y);
    lv_anim_set_var(&a_logo_y, splash_logo_img);
    lv_anim_set_exec_cb(&a_logo_y, slide_y_anim_cb);
    lv_anim_set_values(&a_logo_y, -50, 10); 
    lv_anim_set_time(&a_logo_y, 800);
    lv_anim_set_delay(&a_logo_y, 100);
    lv_anim_set_path_cb(&a_logo_y, lv_anim_path_overshoot);
    lv_anim_start(&a_logo_y);

    lv_anim_init(&a_logo_opa);
    lv_anim_set_var(&a_logo_opa, splash_logo_img);
    lv_anim_set_exec_cb(&a_logo_opa, fade_in_anim_cb);
    lv_anim_set_values(&a_logo_opa, 0, 255);
    lv_anim_set_time(&a_logo_opa, 600);
    lv_anim_set_delay(&a_logo_opa, 100);
    lv_anim_start(&a_logo_opa);

    /* 3. 创建主标题文本 */
    splash_title = lv_label_create(splash_screen);
    lv_label_set_text(splash_title, "Weather Clock");
    lv_obj_set_style_text_color(splash_title, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(splash_title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(splash_title, LV_ALIGN_TOP_MID, 0, 0); 
    lv_obj_set_style_opa(splash_title, 0, LV_PART_MAIN);

    /* --- 文本动画 --- */
    lv_anim_t a_title_y, a_title_opa;
    lv_anim_init(&a_title_y);
    lv_anim_set_var(&a_title_y, splash_title);
    lv_anim_set_exec_cb(&a_title_y, slide_y_anim_cb);
    lv_anim_set_values(&a_title_y, 190, 200); 
    lv_anim_set_time(&a_title_y, 600);
    lv_anim_set_delay(&a_title_y, 400); 
    lv_anim_set_path_cb(&a_title_y, lv_anim_path_ease_out);
    lv_anim_start(&a_title_y);

    lv_anim_init(&a_title_opa);
    lv_anim_set_var(&a_title_opa, splash_title);
    lv_anim_set_exec_cb(&a_title_opa, fade_in_anim_cb);
    lv_anim_set_values(&a_title_opa, 0, 255);
    lv_anim_set_time(&a_title_opa, 600);
    lv_anim_set_delay(&a_title_opa, 400);
    lv_anim_start(&a_title_opa);

    /* 4. 创建底部进度条控件与状态文本 */
    splash_bar_bg = lv_obj_create(splash_screen);
    lv_obj_set_size(splash_bar_bg, 160, 6); 
    lv_obj_align(splash_bar_bg, LV_ALIGN_BOTTOM_MID, 0, -40); // 锁定在底部居中
    lv_obj_set_style_bg_color(splash_bar_bg, lv_color_make(0x2A, 0x2A, 0x2A), LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_bar_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_bar_bg, 3, LV_PART_MAIN);
    lv_obj_set_style_opa(splash_bar_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(splash_bar_bg, 0, LV_PART_MAIN);
    lv_obj_clear_flag(splash_bar_bg, LV_OBJ_FLAG_SCROLLABLE);

    splash_bar = lv_obj_create(splash_bar_bg);
    lv_obj_set_size(splash_bar, 0, 6); 
    lv_obj_align(splash_bar, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(splash_bar, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_pad_all(splash_bar, 0, LV_PART_MAIN);
    lv_obj_clear_flag(splash_bar, LV_OBJ_FLAG_SCROLLABLE);

    // 【新增】创建跟随进度条的百分比文字标签
    splash_progress_label = lv_label_create(splash_screen);
    lv_label_set_text(splash_progress_label, "0%");
    lv_obj_set_style_text_color(splash_progress_label, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(splash_progress_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_opa(splash_progress_label, 0, LV_PART_MAIN);
    // 初始锚定
    lv_obj_align_to(splash_progress_label, splash_bar, LV_ALIGN_OUT_TOP_RIGHT, 10, -8);

    splash_status = lv_label_create(splash_screen);
    lv_label_set_text(splash_status, "Initializing System...");
    lv_obj_set_style_text_color(splash_status, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(splash_status, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(splash_status, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_opa(splash_status, 0, LV_PART_MAIN);

    /* --- 底部组件淡入动画 (背景槽、状态文本、百分比文字一同淡入) --- */
    lv_anim_t a_bar_opa, a_stat_opa, a_perc_opa;
    lv_anim_init(&a_bar_opa);
    lv_anim_set_var(&a_bar_opa, splash_bar_bg);
    lv_anim_set_exec_cb(&a_bar_opa, fade_in_anim_cb);
    lv_anim_set_values(&a_bar_opa, 0, 255);
    lv_anim_set_time(&a_bar_opa, 500);
    lv_anim_set_delay(&a_bar_opa, 800);
    lv_anim_start(&a_bar_opa);

    lv_anim_init(&a_stat_opa);
    lv_anim_set_var(&a_stat_opa, splash_status);
    lv_anim_set_exec_cb(&a_stat_opa, fade_in_anim_cb);
    lv_anim_set_values(&a_stat_opa, 0, 255);
    lv_anim_set_time(&a_stat_opa, 500);
    lv_anim_set_delay(&a_stat_opa, 800);
    lv_anim_start(&a_stat_opa);

    lv_anim_init(&a_perc_opa);
    lv_anim_set_var(&a_perc_opa, splash_progress_label);
    lv_anim_set_exec_cb(&a_perc_opa, fade_in_anim_cb);
    lv_anim_set_values(&a_perc_opa, 0, 255);
    lv_anim_set_time(&a_perc_opa, 500);
    lv_anim_set_delay(&a_perc_opa, 800);
    lv_anim_start(&a_perc_opa);

    /* --- 模拟进度条加载动画 --- */
    lv_anim_t bar_anim;
    lv_anim_init(&bar_anim);
    lv_anim_set_var(&bar_anim, splash_bar);
    lv_anim_set_exec_cb(&bar_anim, splash_bar_anim_cb);
    lv_anim_set_values(&bar_anim, 0, 160); 
    lv_anim_set_time(&bar_anim, 2500); 
    lv_anim_set_delay(&bar_anim, 1000); 
    lv_anim_set_path_cb(&bar_anim, lv_anim_path_ease_in_out); 
    lv_anim_start(&bar_anim);

    lvgl_unlock();
}

/**
 * @brief 更新启动页的加载进度和状态文本
 */
void splash_set_progress(uint8_t progress, const char *status)
{
    lvgl_lock();
    
    splash_progress = progress;
    
    if (splash_bar && progress <= 100) {
        // 如果接入真实进度，立刻杀掉自带的伪动画
        lv_anim_del(splash_bar, NULL);
        
        lv_obj_set_width(splash_bar, (progress * 160) / 100); 
        
        // 【新增】在真实数据推入时，也同步更新跟随文字和位置
        if (splash_progress_label) {
            lv_label_set_text_fmt(splash_progress_label, "%d%%", progress);
            lv_obj_align_to(splash_progress_label, splash_bar, LV_ALIGN_OUT_TOP_RIGHT, 10, -8);
        }
    }
    
    if (splash_status && status) {
        lv_label_set_text(splash_status, status);
    }
    
    lvgl_unlock();
}

/**
 * @brief 结束启动页面，释放相关内存
 */
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
        
        // 清理所有关联指针
        splash_screen = NULL;
        splash_title = NULL;
        splash_bar = NULL;
        splash_bar_bg = NULL;
        splash_status = NULL;
        splash_logo_img = NULL;
        splash_progress_label = NULL; // 【新增】置空指针防止野指针
    }
    
    lvgl_unlock();
}

void welcome_page_display(void)
{
    splash_screen_start();
}

/*============================================================================*/
/* WiFi页实现                                     */
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
/* 错误页实现                                     */
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

    lvgl_unlock();
}

/*============================================================================*/
/* 主界面实现 - 滑动多页面系统                    */
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
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(status_bar, 10, LV_PART_MAIN);
    
    status_wifi_icon = lv_label_create(status_bar);
    lv_label_set_text(status_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(status_wifi_icon, COLOR_ACCENT, LV_PART_MAIN);
    
    status_wifi_label = lv_label_create(status_bar);
    lv_label_set_text(status_wifi_label, WIFI_SSID);
    lv_obj_set_style_text_color(status_wifi_label, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_pad_left(status_wifi_label, 5, LV_PART_MAIN);
}

static void create_time_page(void)
{
    lv_obj_set_flex_flow(time_page, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *time_container = lv_obj_create(time_page);
    lv_obj_set_flex_flow(time_container, LV_FLEX_FLOW_COLUMN);

    time_label = lv_label_create(time_container);
    lv_label_set_text(time_label, "--:--:--"); 

    date_label = lv_label_create(time_container);
    lv_label_set_text(date_label, "----/--/--");

    indoor_card = lv_obj_create(time_page);
    lv_obj_set_flex_flow(indoor_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(indoor_card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); 

    lv_obj_t *indoor_left = lv_obj_create(indoor_card);
    indoor_temp_label = lv_label_create(indoor_left);
    lv_label_set_text(indoor_temp_label, "-- C");

    lv_obj_t *indoor_right = lv_obj_create(indoor_card);
    indoor_humidity_label = lv_label_create(indoor_right);
    lv_label_set_text(indoor_humidity_label, "-- %");

    outdoor_card = lv_obj_create(time_page);
    lv_obj_set_flex_flow(outdoor_card, LV_FLEX_FLOW_ROW);
}

static void create_weather_page(void)
{
    weather_city_label = lv_label_create(weather_page);
    lv_label_set_text(weather_city_label, "Hengyang"); 

    lv_obj_t *temp_container = lv_obj_create(weather_page);
    weather_temp_label = lv_label_create(temp_container);
    lv_label_set_text(weather_temp_label, "--");

    lv_obj_t *temp_unit = lv_label_create(temp_container);
    lv_label_set_text(temp_unit, "C");

    weather_desc_label = lv_label_create(weather_page);
    lv_label_set_text(weather_desc_label, "Loading...");

    weather_detail_card = lv_obj_create(weather_page);
    weather_humidity_label = lv_label_create(weather_detail_card);
    lv_label_set_text(weather_humidity_label, "Humidity: --%");

    weather_wind_label = lv_label_create(weather_detail_card);
    lv_label_set_text(weather_wind_label, "Wind: -- km/h");
}

static void create_settings_page(void)
{
    lv_obj_t *title = lv_label_create(settings_page);
    lv_label_set_text(title, "Settings");

    lv_obj_t *city_row = lv_obj_create(settings_page);
    setting_city_label = lv_label_create(city_row);
    lv_label_set_text(setting_city_label, "Hengyang");

    lv_obj_t *device_row = lv_obj_create(settings_page);
    lv_obj_t *device_val = lv_label_create(device_row);
    lv_label_set_text(device_val, "STM32F407"); 

    lv_obj_t *wifi_row = lv_obj_create(settings_page);
    lv_obj_t *wifi_val = lv_label_create(wifi_row);
    lv_label_set_text(wifi_val, "ESP8266"); 

    lv_obj_t *version_row = lv_obj_create(settings_page);
    setting_version_label = lv_label_create(version_row);
    lv_label_set_text(setting_version_label, "v1.2.0");

    lv_obj_t *hint = lv_label_create(settings_page);
    lv_label_set_text(hint, LV_SYMBOL_LEFT " Slide to switch " LV_SYMBOL_RIGHT);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

static void create_indicator_bar(void)
{
    lv_obj_t *indic = lv_obj_create(lv_scr_act());
    lv_obj_set_size(indic, 60, 10);
    lv_obj_set_pos(indic, (SCREEN_WIDTH - 60) / 2, SCREEN_HEIGHT - INDICATOR_HEIGHT / 2 - 5);
    lv_obj_set_flex_flow(indic, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(indic, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); 
    
    for (int i = 0; i < 3; i++) {
        page_indicator[i] = lv_obj_create(indic);
        lv_obj_set_size(page_indicator[i], 8, 8);
        lv_obj_set_style_bg_color(page_indicator[i], i == 0 ? COLOR_ACCENT : COLOR_INDICATOR_DIM, LV_PART_MAIN);
        lv_obj_set_style_radius(page_indicator[i], 4, LV_PART_MAIN); 
        lv_obj_set_style_opa(page_indicator[i], i == 0 ? 255 : 150, LV_PART_MAIN);
    }
}

/*============================================================================*/
/* 数据更新接口(供系统业务逻辑层调用)             */
/*============================================================================*/

void main_page_redraw_wifi_ssid(const char *ssid, bool connected)
{
    lvgl_lock(); 
    if (status_wifi_label) {
        lv_label_set_text(status_wifi_label, ssid);
    }
    if (status_wifi_icon) {
        if (connected) {
            lv_obj_set_style_text_color(status_wifi_icon, COLOR_ACCENT, LV_PART_MAIN);
        } else {
            lv_obj_set_style_text_color(status_wifi_icon, lv_color_make(0xFF, 0x44, 0x44), LV_PART_MAIN);
        }
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
