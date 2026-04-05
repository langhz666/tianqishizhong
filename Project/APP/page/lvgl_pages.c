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

/* 引入在其他地方(通常是FreeRTOS任务初始化时)创建的LVGL互斥锁，用于保证多线程操作LVGL时的线程安全 */
extern osMutexId_t lcd_mutex;

/*============================================================================*/
/* 静态全局变量                                   */
/*============================================================================*/

/*---------------------------- 背景图片 ----------------------------------*/
static lv_obj_t *bg_image = NULL;               /**< 背景图片对象 */
static lv_img_dsc_t bg_img_dsc;                 /**< 背景图片描述符 */

/*---------------------------- 全局状态栏对象 ----------------------------------*/
static lv_obj_t *status_bar = NULL;             /**< 状态栏容器 */
static lv_obj_t *status_wifi_icon = NULL;       /**< WiFi信号图标文本对象 */
static lv_obj_t *status_wifi_label = NULL;      /**< WiFi SSID文本标签 */

/*---------------------------- 主视图与导航 --------------------------------*/
static lv_obj_t *tabview = NULL;                /**< 滑动标签页视图控制器，用于管理多个页面 */
static lv_obj_t *page_indicator[3] = {NULL};    /**< 底部页面位置指示器(小圆点)数组 */

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
static lv_obj_t *weather_icon_canvas = NULL;    /**< 天气图标画布，用于自定义绘制天气图形 */
static lv_color_t *weather_icon_buf = NULL;     /**< 天气图标画布所在的内存缓冲区 */
static int current_weather_code = -1;           /**< 当前天气代码，用于判断绘制哪种天气图标 */

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
static lv_obj_t *splash_progress_label = NULL;  /**< 跟随进度条的百分比文字 */
static lv_img_dsc_t logo_img_dsc;               /**< LVGL图片描述符结构体，用于加载C数组格式的图片 */
static uint8_t splash_progress = 0;             /**< 当前进度值(0-100) */

/*---------------------------- 系统状态记录 --------------------------------------*/
static uint8_t current_page = 0;                /**< 当前所在的Tab页面索引(0, 1, 2) */

/*============================================================================*/
/* UI 颜色主题定义 (采用RGB888或HEX颜色值构建lv_color_t)              */
/*============================================================================*/

#define COLOR_BG_PRIMARY    lv_color_black()                    /**< 主背景色 (纯黑) */
#define COLOR_BG_CARD       lv_color_make(0x1A, 0x1A, 0x1A)     /**< 普通卡片背景色 (深灰) */
#define COLOR_BG_INNER      lv_color_make(0x2E, 0x7D, 0x32)     /**< 室内环境卡片背景 (墨绿) */
#define COLOR_BG_OUTDOOR    lv_color_make(0xC6, 0x28, 0x28)     /**< 室外环境卡片背景 (深红) */
#define COLOR_BG_WEATHER    lv_color_make(0x0D, 0x47, 0xA1)     /**< 天气专属卡片背景 (深蓝) */
#define COLOR_TEXT_PRIMARY  lv_color_white()                    /**< 主要文本颜色 (纯白) */
#define COLOR_TEXT_SECONDARY lv_color_make(0xAA, 0xAA, 0xAA)    /**< 次要文本颜色 (浅灰) */
#define COLOR_ACCENT        lv_color_make(0x00, 0xD4, 0xAA)     /**< 强调色/主题色 (青绿色) */
#define COLOR_INDICATOR     lv_color_make(0xFF, 0xFF, 0xFF)     /**< 激活状态的指示器颜色 */
#define COLOR_INDICATOR_DIM lv_color_make(0x55, 0x55, 0x55)     /**< 未激活状态的指示器颜色 */

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
/* LVGL线程锁封装 (在FreeRTOS等RTOS下，多个任务调用LVGL API必须加锁) */
/*============================================================================*/

/**
 * @brief 获取LVGL互斥锁，阻塞直到获取成功
 */
static void lvgl_lock(void)
{
    if (lcd_mutex != NULL) {
        osMutexAcquire(lcd_mutex, osWaitForever);
    }
}

/**
 * @brief 释放LVGL互斥锁
 */
static void lvgl_unlock(void)
{
    if (lcd_mutex != NULL) {
        osMutexRelease(lcd_mutex);
    }
}

/*============================================================================*/
/* 动画回调函数实现 (LVGL动画引擎会自动以不断变化的 `v` 值调用这些函数) */
/*============================================================================*/

/**
 * @brief Y轴位移动画回调：动态改变对象的Y坐标
 */
static void slide_y_anim_cb(void *var, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)var, v);
}

/**
 * @brief 淡入淡出动画回调：动态改变对象的不透明度 (0为全透明，255为完全不透明)
 */
static void fade_in_anim_cb(void *var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, v, LV_PART_MAIN);
}

/**
 * @brief 进度条宽度变化回调，同时驱动百分比文字跟随
 * @param v 当前动画输出的值(代表进度条的宽度像素)
 */
static void splash_bar_anim_cb(void *var, int32_t v)
{
    lv_obj_t *bar = (lv_obj_t *)var;
    lv_obj_set_width(bar, v); // 设置进度条前景的宽度

    // 让百分比文字跟随进度条的末端移动
    if (splash_progress_label != NULL) {
        int percent = (v * 100) / 160; // 160是背景槽的最大宽度，计算百分比
        lv_label_set_text_fmt(splash_progress_label, "%d%%", percent); // 更新文字
        
        // 动态对齐：始终锚定在进度条当前宽度的右上角外部，向右偏10像素，向上偏8像素
        lv_obj_align_to(splash_progress_label, bar, LV_ALIGN_OUT_TOP_RIGHT, 10, -8);
    }
}

/*============================================================================*/
/* 天气图标绘制函数 (使用lv_canvas基于像素进行自定义图形绘制)       */
/*============================================================================*/

#define WEATHER_ICON_SIZE 48 // 画布尺寸 48x48

/**
 * @brief 绘制晴天图标 (中间圆圈 + 周围的太阳光线)
 */
static void draw_weather_icon_sunny(lv_obj_t *canvas, lv_color_t color)
{
    // 填充底色(覆盖之前的绘图)
    lv_canvas_fill_bg(canvas, lv_color_make(0xE6, 0x51, 0x00), LV_OPA_COVER);
    
    // 初始化矩形描述符，用于画圆
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = color;
    rect_dsc.radius = LV_RADIUS_CIRCLE; // 将圆角设为最大，矩形即变圆形
    
    // 在中心画一个16x16的圆 (x, y, w, h)
    lv_canvas_draw_rect(canvas, 16, 16, 16, 16, &rect_dsc);
    
    // 初始化线条描述符，用于画太阳的光芒
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = color;
    line_dsc.width = 2; // 线宽2像素
    
    // 定义8条光线的起点和终点坐标
    lv_point_t rays[8][2] = {
        {{24, 16}, {24, 8}},   // 上
        {{32, 24}, {40, 24}},  // 右
        {{24, 32}, {24, 40}},  // 下
        {{16, 24}, {8, 24}},   // 左
        {{30, 14}, {36, 8}},   // 右上
        {{34, 30}, {40, 36}},  // 右下
        {{14, 30}, {8, 36}},   // 左下
        {{14, 18}, {8, 12}}    // 左上
    };
    
    // 循环绘制8条线
    for (int i = 0; i < 8; i++) {
        lv_canvas_draw_line(canvas, rays[i], 2, &line_dsc);
    }
}

/**
 * @brief 绘制多云图标 (由多个圆和圆角矩形拼接而成)
 */
static void draw_weather_icon_cloudy(lv_obj_t *canvas, lv_color_t color)
{
    lv_canvas_fill_bg(canvas, lv_color_make(0xE6, 0x51, 0x00), LV_OPA_COVER);
    
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = color;
    rect_dsc.radius = LV_RADIUS_CIRCLE;
    
    // 画三个圆形构成云朵上半部分凸起
    lv_canvas_draw_rect(canvas, 14, 12, 12, 12, &rect_dsc);
    lv_canvas_draw_rect(canvas, 8, 16, 10, 10, &rect_dsc);
    lv_canvas_draw_rect(canvas, 24, 10, 12, 12, &rect_dsc);
    
    // 画一个底部圆角矩形连接上面的圆，构成完整的云朵形状
    rect_dsc.radius = 8;
    lv_canvas_draw_rect(canvas, 8, 22, 28, 14, &rect_dsc);
}

/**
 * @brief 绘制雨天图标 (上方云朵，下方斜线代表雨滴)
 */
static void draw_weather_icon_rainy(lv_obj_t *canvas, lv_color_t color)
{
    lv_canvas_fill_bg(canvas, lv_color_make(0xE6, 0x51, 0x00), LV_OPA_COVER);
    
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_make(0xB0, 0xBE, 0xC5); // 乌云颜色
    rect_dsc.radius = LV_RADIUS_CIRCLE;
    
    // 绘制乌云
    lv_canvas_draw_rect(canvas, 12, 6, 10, 10, &rect_dsc);
    lv_canvas_draw_rect(canvas, 6, 10, 8, 8, &rect_dsc);
    lv_canvas_draw_rect(canvas, 22, 8, 10, 10, &rect_dsc);
    
    rect_dsc.radius = 6;
    lv_canvas_draw_rect(canvas, 6, 14, 26, 10, &rect_dsc);
    
    // 绘制雨滴（斜线）
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = color;
    line_dsc.width = 2;
    
    lv_point_t rain1[2] = {{14, 28}, {12, 36}};
    lv_point_t rain2[2] = {{24, 28}, {22, 36}};
    lv_point_t rain3[2] = {{34, 28}, {32, 36}};
    
    lv_canvas_draw_line(canvas, rain1, 2, &line_dsc);
    lv_canvas_draw_line(canvas, rain2, 2, &line_dsc);
    lv_canvas_draw_line(canvas, rain3, 2, &line_dsc);
}

/**
 * @brief 绘制雪天图标 (云朵下方飘落白色小点)
 */
static void draw_weather_icon_snowy(lv_obj_t *canvas, lv_color_t color)
{
    lv_canvas_fill_bg(canvas, lv_color_make(0xE6, 0x51, 0x00), LV_OPA_COVER);
    
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_make(0xB0, 0xBE, 0xC5);
    rect_dsc.radius = LV_RADIUS_CIRCLE;
    
    // 绘制乌云
    lv_canvas_draw_rect(canvas, 12, 6, 10, 10, &rect_dsc);
    lv_canvas_draw_rect(canvas, 6, 10, 8, 8, &rect_dsc);
    lv_canvas_draw_rect(canvas, 22, 8, 10, 10, &rect_dsc);
    
    rect_dsc.radius = 6;
    lv_canvas_draw_rect(canvas, 6, 14, 26, 10, &rect_dsc);
    
    // 绘制雪花点（小圆形）
    rect_dsc.bg_color = color;
    rect_dsc.radius = LV_RADIUS_CIRCLE;
    
    lv_canvas_draw_rect(canvas, 12, 30, 4, 4, &rect_dsc);
    lv_canvas_draw_rect(canvas, 22, 34, 4, 4, &rect_dsc);
    lv_canvas_draw_rect(canvas, 32, 30, 4, 4, &rect_dsc);
}

/**
 * @brief 绘制雷雨天图标 (云朵下方画一个闪电折线)
 */
static void draw_weather_icon_thunder(lv_obj_t *canvas, lv_color_t color)
{
    lv_canvas_fill_bg(canvas, lv_color_make(0xE6, 0x51, 0x00), LV_OPA_COVER);
    
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_make(0x78, 0x90, 0x9C);
    rect_dsc.radius = LV_RADIUS_CIRCLE;
    
    // 绘制深色乌云
    lv_canvas_draw_rect(canvas, 12, 4, 10, 10, &rect_dsc);
    lv_canvas_draw_rect(canvas, 6, 8, 8, 8, &rect_dsc);
    lv_canvas_draw_rect(canvas, 22, 6, 10, 10, &rect_dsc);
    
    rect_dsc.radius = 6;
    lv_canvas_draw_rect(canvas, 6, 12, 26, 10, &rect_dsc);
    
    // 绘制闪电
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = color;
    line_dsc.width = 3;
    
    // 闪电的四个折点
    lv_point_t bolt[4] = {{26, 24}, {20, 32}, {24, 32}, {18, 44}};
    lv_canvas_draw_line(canvas, bolt, 4, &line_dsc); // 画连续折线
}

/**
 * @brief 绘制夜晚月亮图标 (画一个圆，然后用背景色画一个小一点的圆遮挡，形成月牙)
 */
static void draw_weather_icon_night(lv_obj_t *canvas, lv_color_t color)
{
    lv_canvas_fill_bg(canvas, lv_color_make(0xE6, 0x51, 0x00), LV_OPA_COVER);
    
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = color;
    rect_dsc.radius = LV_RADIUS_CIRCLE;
    
    // 画完整的月亮（底层亮色圆）
    lv_canvas_draw_rect(canvas, 10, 6, 20, 20, &rect_dsc);
    
    // 画遮挡圆（用背景色覆盖右上角，形成新月形状）
    rect_dsc.bg_color = lv_color_make(0xE6, 0x51, 0x00);
    lv_canvas_draw_rect(canvas, 18, 4, 20, 20, &rect_dsc);
}

/**
 * @brief 未知天气容错绘制 (直接写 "??" 文字)
 */
static void draw_weather_icon_unknown(lv_obj_t *canvas, lv_color_t color)
{
    lv_canvas_fill_bg(canvas, lv_color_make(0xE6, 0x51, 0x00), LV_OPA_COVER);
    
    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = color;
    label_dsc.font = &lv_font_montserrat_20;
    
    lv_canvas_draw_text(canvas, 12, 14, 24, &label_dsc, "??");
}

/**
 * @brief 核心天气图标分发函数：根据传入的气象代码映射到相应的绘制函数
 */
static void draw_weather_icon(int code)
{
    if (weather_icon_canvas == NULL || weather_icon_buf == NULL) return;
    
    lv_color_t icon_color = lv_color_make(0xFF, 0xCC, 0xBC);
    
    // 根据具体气象API返回的代码做分类映射 (这里假设是一套通用的气象代码)
    if (code == 0 || code == 2 || code == 38) {
        draw_weather_icon_sunny(weather_icon_canvas, icon_color);
    } else if (code == 1 || code == 3) {
        draw_weather_icon_night(weather_icon_canvas, icon_color);
    } else if (code == 4 || code == 9 || code == 30) {
        draw_weather_icon_cloudy(weather_icon_canvas, icon_color);
    } else if (code == 5 || code == 6 || code == 7 || code == 8) {
        draw_weather_icon_cloudy(weather_icon_canvas, icon_color);
    } else if (code >= 10 && code <= 19) {
        draw_weather_icon_rainy(weather_icon_canvas, icon_color);
    } else if (code == 11 || code == 12) {
        draw_weather_icon_thunder(weather_icon_canvas, icon_color);
    } else if (code >= 20 && code <= 25) {
        draw_weather_icon_snowy(weather_icon_canvas, icon_color);
    } else {
        draw_weather_icon_unknown(weather_icon_canvas, icon_color);
    }
}

/*============================================================================*/
/* UI事件回调函数                                 */
/*============================================================================*/

/**
 * @brief Tabview页面切换事件回调
 * 当用户滑动屏幕或调用API切换页面时触发
 */
static void tabview_switch_cb(lv_event_t * e)
{
    lv_obj_t *tv = lv_event_get_target(e);        // 获取触发事件的对象(即tabview)
    uint16_t active = lv_tabview_get_tab_act(tv); // 获取当前激活的页面索引
    current_page = active;                        // 更新全局变量
    update_page_indicator(active);                // 联动更新底部小圆点指示器
}

/**
 * @brief 更新底部的页面指示器(小圆点)
 * @param page 当前高亮的页面索引
 */
static void update_page_indicator(uint8_t page)
{
    for (int i = 0; i < 3; i++) {
        if (page_indicator[i] != NULL) {
            if (i == page) {
                // 当前页面对应的小圆点：设为主题色，完全不透明
                lv_obj_set_style_bg_color(page_indicator[i], COLOR_ACCENT, LV_PART_MAIN);
                lv_obj_set_style_opa(page_indicator[i], 255, LV_PART_MAIN);
            } else {
                // 其他页面对应的小圆点：设为暗灰色，半透明
                lv_obj_set_style_bg_color(page_indicator[i], COLOR_INDICATOR_DIM, LV_PART_MAIN);
                lv_obj_set_style_opa(page_indicator[i], 150, LV_PART_MAIN);
            }
        }
    }
}

/*============================================================================*/
/* 欢迎/启动页实现                                */
/*============================================================================*/

/**
 * @brief 启动进入欢迎页面 (创建UI元素并启动进入动画)
 */
void splash_screen_start(void)
{
    lvgl_lock(); // 上锁，保护UI操作

    // 清空当前活动屏幕上的所有对象，设置背景色为纯黑
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG_PRIMARY, LV_PART_MAIN);

    /* 1. 创建全屏背景容器 */
    splash_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(splash_screen, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(splash_screen, COLOR_BG_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_screen, 0, LV_PART_MAIN); // 去除默认边框
    lv_obj_set_style_radius(splash_screen, 0, LV_PART_MAIN);       // 去除默认圆角
    lv_obj_clear_flag(splash_screen, LV_OBJ_FLAG_SCROLLABLE);      // 禁止滚动

    /* 2. 挂载并配置 Logo 图片 */
    // 初始化LVGL图片描述符，以便将从C数组加载过来的图片显示在屏幕上
    logo_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    logo_img_dsc.header.always_zero = 0;
    logo_img_dsc.header.reserved = 0;
    logo_img_dsc.header.w = img_chengpingan.width;
    logo_img_dsc.header.h = img_chengpingan.height;
    logo_img_dsc.data_size = img_chengpingan.width * img_chengpingan.height * 2; // RGB565占2字节
    logo_img_dsc.data = img_chengpingan.data;

    splash_logo_img = lv_img_create(splash_screen);
    lv_img_set_src(splash_logo_img, &logo_img_dsc); // 设置图片源
    
    // 初始位置设定在顶部居中，先设置为全透明，准备做淡入动画
    lv_obj_align(splash_logo_img, LV_ALIGN_TOP_MID, 0, 0); 
    lv_obj_set_style_opa(splash_logo_img, 0, LV_PART_MAIN);

    /* --- Logo 动画 --- */
    // a_logo_y: 实现从上方 -50 位置下滑到 +10 的效果 (弹性缓动)
    lv_anim_t a_logo_y, a_logo_opa;
    lv_anim_init(&a_logo_y);
    lv_anim_set_var(&a_logo_y, splash_logo_img);
    lv_anim_set_exec_cb(&a_logo_y, slide_y_anim_cb);
    lv_anim_set_values(&a_logo_y, -50, 10); 
    lv_anim_set_time(&a_logo_y, 800);
    lv_anim_set_delay(&a_logo_y, 100);
    lv_anim_set_path_cb(&a_logo_y, lv_anim_path_overshoot); // 动画曲线：超过一点再弹回来
    lv_anim_start(&a_logo_y);

    // a_logo_opa: 实现从透明(0)到不透明(255)的淡入效果
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

    /* --- 文本动画 (类似Logo动画，稍微延迟出现) --- */
    lv_anim_t a_title_y, a_title_opa;
    lv_anim_init(&a_title_y);
    lv_anim_set_var(&a_title_y, splash_title);
    lv_anim_set_exec_cb(&a_title_y, slide_y_anim_cb);
    lv_anim_set_values(&a_title_y, 190, 200); 
    lv_anim_set_time(&a_title_y, 600);
    lv_anim_set_delay(&a_title_y, 400); 
    lv_anim_set_path_cb(&a_title_y, lv_anim_path_ease_out); // 动画曲线：逐渐变慢
    lv_anim_start(&a_title_y);

    lv_anim_init(&a_title_opa);
    lv_anim_set_var(&a_title_opa, splash_title);
    lv_anim_set_exec_cb(&a_title_opa, fade_in_anim_cb);
    lv_anim_set_values(&a_title_opa, 0, 255);
    lv_anim_set_time(&a_title_opa, 600);
    lv_anim_set_delay(&a_title_opa, 400);
    lv_anim_start(&a_title_opa);

    /* 4. 创建底部进度条控件与状态文本 */
    // 进度条背景(空槽)
    splash_bar_bg = lv_obj_create(splash_screen);
    lv_obj_set_size(splash_bar_bg, 160, 6); 
    lv_obj_align(splash_bar_bg, LV_ALIGN_BOTTOM_MID, 0, -40); // 锁定在底部居中
    lv_obj_set_style_bg_color(splash_bar_bg, lv_color_make(0x2A, 0x2A, 0x2A), LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_bar_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_bar_bg, 3, LV_PART_MAIN);
    lv_obj_set_style_opa(splash_bar_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(splash_bar_bg, 0, LV_PART_MAIN); // 取消内部填充
    lv_obj_clear_flag(splash_bar_bg, LV_OBJ_FLAG_SCROLLABLE);

    // 进度条前景(实心填充部分)，创建为背景的子对象
    splash_bar = lv_obj_create(splash_bar_bg);
    lv_obj_set_size(splash_bar, 0, 6);  // 初始宽度为0
    lv_obj_align(splash_bar, LV_ALIGN_LEFT_MID, 0, 0); // 左侧对齐，随宽度增加向右生长
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
    // 初始锚定位置
    lv_obj_align_to(splash_progress_label, splash_bar, LV_ALIGN_OUT_TOP_RIGHT, 10, -8);

    // 状态文本("Initializing System...")
    splash_status = lv_label_create(splash_screen);
    lv_label_set_text(splash_status, "Initializing System...");
    lv_obj_set_style_text_color(splash_status, COLOR_TEXT_SECONDARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(splash_status, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(splash_status, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_opa(splash_status, 0, LV_PART_MAIN);

    /* --- 底部组件淡入动画 (背景槽、状态文本、百分比文字一同淡入) --- */
    lv_anim_t a_bar_opa, a_stat_opa, a_perc_opa;
    
    // 淡入进度条背景
    lv_anim_init(&a_bar_opa);
    lv_anim_set_var(&a_bar_opa, splash_bar_bg);
    lv_anim_set_exec_cb(&a_bar_opa, fade_in_anim_cb);
    lv_anim_set_values(&a_bar_opa, 0, 255);
    lv_anim_set_time(&a_bar_opa, 500);
    lv_anim_set_delay(&a_bar_opa, 800);
    lv_anim_start(&a_bar_opa);

    // 淡入状态文本
    lv_anim_init(&a_stat_opa);
    lv_anim_set_var(&a_stat_opa, splash_status);
    lv_anim_set_exec_cb(&a_stat_opa, fade_in_anim_cb);
    lv_anim_set_values(&a_stat_opa, 0, 255);
    lv_anim_set_time(&a_stat_opa, 500);
    lv_anim_set_delay(&a_stat_opa, 800);
    lv_anim_start(&a_stat_opa);

    // 淡入百分比文本
    lv_anim_init(&a_perc_opa);
    lv_anim_set_var(&a_perc_opa, splash_progress_label);
    lv_anim_set_exec_cb(&a_perc_opa, fade_in_anim_cb);
    lv_anim_set_values(&a_perc_opa, 0, 255);
    lv_anim_set_time(&a_perc_opa, 500);
    lv_anim_set_delay(&a_perc_opa, 800);
    lv_anim_start(&a_perc_opa);

    /* --- 模拟进度条加载动画 --- 
     * 若系统启动时间较短，此动画可提供"正在努力加载"的视觉反馈
     * 当外部调用 `splash_set_progress` 传入真实进度时，此动画会被打断。
     */
    lv_anim_t bar_anim;
    lv_anim_init(&bar_anim);
    lv_anim_set_var(&bar_anim, splash_bar);
    lv_anim_set_exec_cb(&bar_anim, splash_bar_anim_cb);
    lv_anim_set_values(&bar_anim, 0, 160);              // 从宽0加载到宽160(满槽)
    lv_anim_set_time(&bar_anim, 2500);                  // 模拟耗时2.5秒
    lv_anim_set_delay(&bar_anim, 1000); 
    lv_anim_set_path_cb(&bar_anim, lv_anim_path_ease_in_out); 
    lv_anim_start(&bar_anim);

    lvgl_unlock(); // 解锁
}

/**
 * @brief 更新启动页的加载进度和状态文本（外部接口）
 * @param progress 进度百分比(0~100)
 * @param status 进度状态字符串(如："Connecting WiFi...")
 */
void splash_set_progress(uint8_t progress, const char *status)
{
    lvgl_lock();
    
    splash_progress = progress;
    
    if (splash_bar && progress <= 100) {
        // 如果接入真实进度，立刻杀掉自带的伪加载动画，以真实进度为准
        lv_anim_del(splash_bar, NULL);
        
        // 计算真实物理宽度: 真实进度 * 最大宽度(160) / 100
        lv_obj_set_width(splash_bar, (progress * 160) / 100); 
        
        // 【新增】在真实数据推入时，也同步更新跟随文字和位置
        if (splash_progress_label) {
            lv_label_set_text_fmt(splash_progress_label, "%d%%", progress);
            lv_obj_align_to(splash_progress_label, splash_bar, LV_ALIGN_OUT_TOP_RIGHT, 10, -8);
        }
    }
    
    // 更新状态提示文本
    if (splash_status && status) {
        lv_label_set_text(splash_status, status);
    }
    
    lvgl_unlock();
}

/**
 * @brief 结束启动页面，释放相关内存并清空指针
 */
void splash_screen_end(void)
{
    lvgl_lock();
    
    if (splash_screen) {
        // 为退场添加一个短暂的淡出动画，让界面过渡更平滑
        lv_anim_t fade_out;
        lv_anim_init(&fade_out);
        lv_anim_set_var(&fade_out, splash_screen);
        lv_anim_set_exec_cb(&fade_out, fade_in_anim_cb);
        lv_anim_set_values(&fade_out, 255, 0); 
        lv_anim_set_time(&fade_out, 500);
        lv_anim_start(&fade_out);

        // 延迟600ms(等动画播完)后真正删除对象，释放内存
        lv_obj_del_delayed(splash_screen, 600);
        
        // 清理所有关联指针，防止形成野指针导致崩溃
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

/**
 * @brief 暴露给外部的启动页调用接口
 */
void welcome_page_display(void)
{
    splash_screen_start();
}

/*============================================================================*/
/* WiFi配网页实现                                 */
/*============================================================================*/

/**
 * @brief 显示独立的WiFi连接中提示页面
 */
void wifi_page_display(void)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act()); // 清理屏幕
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG_PRIMARY, LV_PART_MAIN);

    // 1. 创建WiFi图标符号 (利用内置的LV_SYMBOL字体)
    lv_obj_t *icon = lv_label_create(lv_scr_act());
    lv_label_set_text(icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(icon, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -60);
    lv_obj_set_style_opa(icon, 0, LV_PART_MAIN);

    // 2. 显示目标 SSID 名称
    lv_obj_t *ssid_label = lv_label_create(lv_scr_act());
    lv_label_set_text(ssid_label, WIFI_SSID); 
    lv_obj_set_style_text_color(ssid_label, COLOR_TEXT_PRIMARY, LV_PART_MAIN);
    lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_opa(ssid_label, 0, LV_PART_MAIN);

    // 3. 当前连接状态的文本提示
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
        lv_anim_set_delay(&anims[i], i * 150); // 通过乘以索引形成阶梯感
        lv_anim_start(&anims[i]);
    }

    lvgl_unlock();
}

/**
 * @brief 更新WiFi配网界面的连接状态文本及颜色
 */
void wifi_page_set_status(const char *status, bool success)
{
    lvgl_lock();
    if (wifi_status_label) {
        lv_label_set_text(wifi_status_label, status);
        if (success) {
            // 成功时显示绿色
            lv_obj_set_style_text_color(wifi_status_label, lv_color_make(0x00, 0xFF, 0x00), LV_PART_MAIN); 
        } else {
            // 失败时显示红色
            lv_obj_set_style_text_color(wifi_status_label, lv_color_make(0xFF, 0x44, 0x44), LV_PART_MAIN); 
        }
    }
    lvgl_unlock();
}

/*============================================================================*/
/* 错误提示页实现                                 */
/*============================================================================*/

/**
 * @brief 全屏显示网络/系统错误提示的页面
 * @param msg 需展示的错误字符串
 */
void error_page_display(const char *msg)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG_PRIMARY, LV_PART_MAIN);

    // 警告图标
    lv_obj_t *icon = lv_label_create(lv_scr_act());
    lv_label_set_text(icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(icon, lv_color_make(0xFF, 0x44, 0x44), LV_PART_MAIN);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_opa(icon, 0, LV_PART_MAIN);

    // 错误信息文本段落
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, msg);
    lv_obj_set_style_text_color(label, lv_color_make(0xFF, 0xAA, 0x00), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_width(label, 200); // 限制宽度为200像素
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP); // 超过200像素则自动换行
    lv_obj_set_style_opa(label, 0, LV_PART_MAIN);

    lvgl_unlock();
}

/*============================================================================*/
/* 主界面实现 - 滑动多页面系统 (Tabview)                  */
/*============================================================================*/

/**
 * @brief 核心函数：创建主UI骨架，包括顶部状态栏、中部滑动视图及三张子页面，和底部指示器
 */
void main_page_display(void)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act());

    bg_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    bg_img_dsc.header.always_zero = 0;
    bg_img_dsc.header.reserved = 0;
    bg_img_dsc.header.w = 240;
    bg_img_dsc.header.h = 320;
    bg_img_dsc.data_size = 240 * 320 * 2;
    bg_img_dsc.data = gImage_xiaozhang;

    bg_image = lv_img_create(lv_scr_act());
    lv_img_set_src(bg_image, &bg_img_dsc);
    lv_obj_set_pos(bg_image, 0, 0);

    create_status_bar();

    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_LEFT, 0); 
    lv_obj_set_size(tabview, SCREEN_WIDTH, CONTENT_HEIGHT);
    lv_obj_set_pos(tabview, 0, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_opa(tabview, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(tabview, 0, LV_PART_MAIN);
    lv_obj_clear_flag(tabview, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_add_event_cb(tabview, tabview_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

    time_page = lv_tabview_add_tab(tabview, "Time");
    weather_page = lv_tabview_add_tab(tabview, "Weather");
    settings_page = lv_tabview_add_tab(tabview, "Settings");
    
    lv_obj_set_style_bg_opa(time_page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(weather_page, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(settings_page, LV_OPA_TRANSP, LV_PART_MAIN);
    
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

/**
 * @brief 创建主界面顶部状态栏(24像素高，横向Flex布局)
 */
static void create_status_bar(void)
{
    status_bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(status_bar, SCREEN_WIDTH, STATUS_BAR_HEIGHT);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(status_bar, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    
    status_wifi_icon = lv_label_create(status_bar);
    lv_label_set_text(status_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(status_wifi_icon, lv_color_white(), LV_PART_MAIN);
    
    status_wifi_label = lv_label_create(status_bar);
    lv_label_set_text(status_wifi_label, WIFI_SSID);
    lv_obj_set_style_text_color(status_wifi_label, lv_color_make(0xCC, 0xCC, 0xCC), LV_PART_MAIN);
    lv_obj_set_style_pad_left(status_wifi_label, 5, LV_PART_MAIN);
}

/**
 * @brief 创建第1个页面内容：大号时钟显示与室内外温湿度简略卡片
 */
static void create_time_page(void)
{
    lv_obj_set_style_pad_all(time_page, 0, LV_PART_MAIN);
    lv_obj_clear_flag(time_page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *time_section = lv_obj_create(time_page);
    lv_obj_set_size(time_section, SCREEN_WIDTH - 16, 90);
    lv_obj_set_pos(time_section, 8, 8);
    lv_obj_set_style_bg_opa(time_section, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_bg_color(time_section, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(time_section, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(time_section, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(time_section, 0, LV_PART_MAIN);
    lv_obj_clear_flag(time_section, LV_OBJ_FLAG_SCROLLABLE);

    time_label = lv_label_create(time_section);
    lv_label_set_text(time_label, "--:--:--");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_38, LV_PART_MAIN);
    lv_obj_set_style_text_color(time_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -8);

    date_label = lv_label_create(time_section);
    lv_label_set_text(date_label, "----/--/-- ---");
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(date_label, lv_color_make(0xCC, 0xCC, 0xCC), LV_PART_MAIN);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 28);

    indoor_card = lv_obj_create(time_page);
    lv_obj_set_size(indoor_card, SCREEN_WIDTH - 16, 72);
    lv_obj_set_pos(indoor_card, 8, 106);
    lv_obj_set_style_bg_opa(indoor_card, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_bg_color(indoor_card, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(indoor_card, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(indoor_card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(indoor_card, 0, LV_PART_MAIN);
    lv_obj_clear_flag(indoor_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *indoor_icon = lv_label_create(indoor_card);
    lv_label_set_text(indoor_icon, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(indoor_icon, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(indoor_icon, lv_color_make(0x90, 0xCA, 0xF9), LV_PART_MAIN);
    lv_obj_set_pos(indoor_icon, 12, 8);

    lv_obj_t *indoor_title = lv_label_create(indoor_card);
    lv_label_set_text(indoor_title, "Indoor");
    lv_obj_set_style_text_font(indoor_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(indoor_title, lv_color_make(0xBB, 0xDE, 0xFB), LV_PART_MAIN);
    lv_obj_set_pos(indoor_title, 38, 12);

    lv_obj_t *indoor_data_row = lv_obj_create(indoor_card);
    lv_obj_set_size(indoor_data_row, SCREEN_WIDTH - 40, 32);
    lv_obj_set_pos(indoor_data_row, 8, 36);
    lv_obj_set_style_bg_opa(indoor_data_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(indoor_data_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(indoor_data_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(indoor_data_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(indoor_data_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(indoor_data_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *temp_col = lv_obj_create(indoor_data_row);
    lv_obj_set_size(temp_col, 90, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(temp_col, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(temp_col, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(temp_col, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(temp_col, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(temp_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(temp_col, LV_OBJ_FLAG_SCROLLABLE);

    indoor_temp_label = lv_label_create(temp_col);
    lv_label_set_text(indoor_temp_label, "--");
    lv_obj_set_style_text_font(indoor_temp_label, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(indoor_temp_label, lv_color_white(), LV_PART_MAIN);

    lv_obj_t *temp_unit = lv_label_create(temp_col);
    lv_label_set_text(temp_unit, "C");
    lv_obj_set_style_text_font(temp_unit, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(temp_unit, lv_color_make(0xBB, 0xDE, 0xFB), LV_PART_MAIN);
    lv_obj_set_pos(temp_unit, 0, -6);

    lv_obj_t *humid_col = lv_obj_create(indoor_data_row);
    lv_obj_set_size(humid_col, 90, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(humid_col, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(humid_col, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(humid_col, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(humid_col, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(humid_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(humid_col, LV_OBJ_FLAG_SCROLLABLE);

    indoor_humidity_label = lv_label_create(humid_col);
    lv_label_set_text(indoor_humidity_label, "--");
    lv_obj_set_style_text_font(indoor_humidity_label, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_color(indoor_humidity_label, lv_color_white(), LV_PART_MAIN);

    lv_obj_t *humid_unit = lv_label_create(humid_col);
    lv_label_set_text(humid_unit, "%");
    lv_obj_set_style_text_font(humid_unit, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(humid_unit, lv_color_make(0xBB, 0xDE, 0xFB), LV_PART_MAIN);
    lv_obj_set_pos(humid_unit, 0, -6);

    outdoor_card = lv_obj_create(time_page);
    lv_obj_set_size(outdoor_card, SCREEN_WIDTH - 16, 72);
    lv_obj_set_pos(outdoor_card, 8, 186);
    lv_obj_set_style_bg_opa(outdoor_card, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_bg_color(outdoor_card, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(outdoor_card, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(outdoor_card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(outdoor_card, 0, LV_PART_MAIN);
    lv_obj_clear_flag(outdoor_card, LV_OBJ_FLAG_SCROLLABLE);

    weather_icon_buf = lv_mem_alloc(WEATHER_ICON_SIZE * WEATHER_ICON_SIZE * sizeof(lv_color_t));
    if (weather_icon_buf) {
        memset(weather_icon_buf, 0, WEATHER_ICON_SIZE * WEATHER_ICON_SIZE * sizeof(lv_color_t));
    }
    
    weather_icon_canvas = lv_canvas_create(outdoor_card);
    lv_canvas_set_buffer(weather_icon_canvas, weather_icon_buf, WEATHER_ICON_SIZE, WEATHER_ICON_SIZE, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(weather_icon_canvas, LV_ALIGN_RIGHT_MID, -8, 0);
    
    draw_weather_icon(0);

    lv_obj_t *outdoor_title = lv_label_create(outdoor_card);
    lv_label_set_text(outdoor_title, "Outdoor");
    lv_obj_set_style_text_font(outdoor_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(outdoor_title, lv_color_make(0xFF, 0xCC, 0x80), LV_PART_MAIN);
    lv_obj_set_pos(outdoor_title, 12, 8);

    outdoor_temp_label = lv_label_create(outdoor_card);
    lv_label_set_text(outdoor_temp_label, "--");
    lv_obj_set_style_text_font(outdoor_temp_label, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(outdoor_temp_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(outdoor_temp_label, 12, 24);

    lv_obj_t *outdoor_unit = lv_label_create(outdoor_card);
    lv_label_set_text(outdoor_unit, "oC");
    lv_obj_set_style_text_font(outdoor_unit, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(outdoor_unit, lv_color_make(0xFF, 0xCC, 0x80), LV_PART_MAIN);
    lv_obj_set_pos(outdoor_unit, 70, 28);

    weather_icon_label = lv_label_create(outdoor_card);
    lv_label_set_text(weather_icon_label, "Loading...");
    lv_obj_set_style_text_font(weather_icon_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(weather_icon_label, lv_color_make(0xFF, 0xCC, 0x80), LV_PART_MAIN);
    lv_obj_set_pos(weather_icon_label, 12, 56);
}

/**
 * @brief 创建第2个页面内容：更详细的天气预报（包含城市、巨大温度、风速湿度）
 */
static void create_weather_page(void)
{
    lv_obj_set_style_pad_all(weather_page, 0, LV_PART_MAIN);
    lv_obj_clear_flag(weather_page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header_section = lv_obj_create(weather_page);
    lv_obj_set_size(header_section, SCREEN_WIDTH - 16, 50);
    lv_obj_set_pos(header_section, 8, 8);
    lv_obj_set_style_bg_opa(header_section, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_bg_color(header_section, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(header_section, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(header_section, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header_section, 0, LV_PART_MAIN);
    lv_obj_clear_flag(header_section, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *location_icon = lv_label_create(header_section);
    lv_label_set_text(location_icon, LV_SYMBOL_GPS);
    lv_obj_set_style_text_font(location_icon, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(location_icon, lv_color_make(0x4F, 0xC3, 0xF7), LV_PART_MAIN);
    lv_obj_set_pos(location_icon, 12, 18);

    weather_city_label = lv_label_create(header_section);
    lv_label_set_text(weather_city_label, "Hengyang");
    lv_obj_set_style_text_font(weather_city_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(weather_city_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(weather_city_label, 32, 16);

    lv_obj_t *temp_section = lv_obj_create(weather_page);
    lv_obj_set_size(temp_section, SCREEN_WIDTH - 16, 100);
    lv_obj_set_pos(temp_section, 8, 66);
    lv_obj_set_style_bg_opa(temp_section, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_bg_color(temp_section, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(temp_section, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(temp_section, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(temp_section, 0, LV_PART_MAIN);
    lv_obj_clear_flag(temp_section, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *temp_row = lv_obj_create(temp_section);
    lv_obj_set_size(temp_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(temp_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(temp_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(temp_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(temp_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(temp_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(temp_row, LV_ALIGN_CENTER, 0, -8);
    lv_obj_clear_flag(temp_row, LV_OBJ_FLAG_SCROLLABLE);

    weather_temp_label = lv_label_create(temp_row);
    lv_label_set_text(weather_temp_label, "--");
    lv_obj_set_style_text_font(weather_temp_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(weather_temp_label, lv_color_white(), LV_PART_MAIN);

    lv_obj_t *temp_unit = lv_label_create(temp_row);
    lv_label_set_text(temp_unit, "C");
    lv_obj_set_style_text_font(temp_unit, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(temp_unit, lv_color_make(0x90, 0xCA, 0xF9), LV_PART_MAIN);
    lv_obj_set_pos(temp_unit, 0, -12);

    weather_desc_label = lv_label_create(temp_section);
    lv_label_set_text(weather_desc_label, "Loading...");
    lv_obj_set_style_text_font(weather_desc_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(weather_desc_label, lv_color_make(0xBB, 0xDE, 0xFB), LV_PART_MAIN);
    lv_obj_align(weather_desc_label, LV_ALIGN_BOTTOM_MID, 0, -10);

    weather_detail_card = lv_obj_create(weather_page);
    lv_obj_set_size(weather_detail_card, SCREEN_WIDTH - 16, 90);
    lv_obj_set_pos(weather_detail_card, 8, 174);
    lv_obj_set_style_bg_opa(weather_detail_card, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_bg_color(weather_detail_card, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(weather_detail_card, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(weather_detail_card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(weather_detail_card, 0, LV_PART_MAIN);
    lv_obj_clear_flag(weather_detail_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *humid_row = lv_obj_create(weather_detail_card);
    lv_obj_set_size(humid_row, SCREEN_WIDTH - 32, 36);
    lv_obj_set_pos(humid_row, 8, 8);
    lv_obj_set_style_bg_opa(humid_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(humid_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(humid_row, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(humid_row, lv_color_make(0x55, 0x55, 0x55), LV_PART_MAIN);
    lv_obj_set_style_border_width(humid_row, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(humid_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(humid_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(humid_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(humid_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *humid_icon = lv_label_create(humid_row);
    lv_label_set_text(humid_icon, LV_SYMBOL_TINT);
    lv_obj_set_style_text_font(humid_icon, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(humid_icon, lv_color_make(0x4F, 0xC3, 0xF7), LV_PART_MAIN);

    lv_obj_t *humid_title = lv_label_create(humid_row);
    lv_label_set_text(humid_title, "  Humidity");
    lv_obj_set_style_text_font(humid_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(humid_title, lv_color_make(0xAA, 0xAA, 0xAA), LV_PART_MAIN);

    weather_humidity_label = lv_label_create(humid_row);
    lv_label_set_text(weather_humidity_label, "--%");
    lv_obj_set_style_text_font(weather_humidity_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(weather_humidity_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(weather_humidity_label, SCREEN_WIDTH - 80, 8);

    lv_obj_t *wind_row = lv_obj_create(weather_detail_card);
    lv_obj_set_size(wind_row, SCREEN_WIDTH - 32, 36);
    lv_obj_set_pos(wind_row, 8, 48);
    lv_obj_set_style_bg_opa(wind_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(wind_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wind_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(wind_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wind_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(wind_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *wind_icon = lv_label_create(wind_row);
    lv_label_set_text(wind_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(wind_icon, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(wind_icon, lv_color_make(0x81, 0xC7, 0x84), LV_PART_MAIN);

    lv_obj_t *wind_title = lv_label_create(wind_row);
    lv_label_set_text(wind_title, "  Wind");
    lv_obj_set_style_text_font(wind_title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(wind_title, lv_color_make(0xAA, 0xAA, 0xAA), LV_PART_MAIN);

    weather_wind_label = lv_label_create(wind_row);
    lv_label_set_text(weather_wind_label, "-- km/h");
    lv_obj_set_style_text_font(weather_wind_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(weather_wind_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(weather_wind_label, SCREEN_WIDTH - 80, 8);
}

/**
 * @brief 创建第3个页面内容：系统设置与硬件信息展示列表
 */
static void create_settings_page(void)
{
    lv_obj_set_style_pad_all(settings_page, 0, LV_PART_MAIN);
    lv_obj_clear_flag(settings_page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header_section = lv_obj_create(settings_page);
    lv_obj_set_size(header_section, SCREEN_WIDTH - 16, 45);
    lv_obj_set_pos(header_section, 8, 8);
    lv_obj_set_style_bg_opa(header_section, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_bg_color(header_section, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(header_section, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(header_section, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header_section, 0, LV_PART_MAIN);
    lv_obj_clear_flag(header_section, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *settings_icon = lv_label_create(header_section);
    lv_label_set_text(settings_icon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(settings_icon, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_icon, lv_color_make(0xCE, 0x93, 0xD8), LV_PART_MAIN);
    lv_obj_set_pos(settings_icon, 12, 13);

    lv_obj_t *title = lv_label_create(header_section);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(title, 38, 12);

    lv_obj_t *info_card = lv_obj_create(settings_page);
    lv_obj_set_size(info_card, SCREEN_WIDTH - 16, 180);
    lv_obj_set_pos(info_card, 8, 61);
    lv_obj_set_style_bg_opa(info_card, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_bg_color(info_card, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(info_card, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(info_card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(info_card, 0, LV_PART_MAIN);
    lv_obj_clear_flag(info_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *city_row = lv_obj_create(info_card);
    lv_obj_set_size(city_row, SCREEN_WIDTH - 32, 40);
    lv_obj_set_pos(city_row, 8, 8);
    lv_obj_set_style_bg_opa(city_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(city_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(city_row, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(city_row, lv_color_make(0x55, 0x55, 0x55), LV_PART_MAIN);
    lv_obj_set_style_border_width(city_row, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(city_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(city_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(city_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(city_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *city_icon = lv_label_create(city_row);
    lv_label_set_text(city_icon, LV_SYMBOL_GPS);
    lv_obj_set_style_text_font(city_icon, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(city_icon, lv_color_make(0x4F, 0xC3, 0xF7), LV_PART_MAIN);

    lv_obj_t *city_label = lv_label_create(city_row);
    lv_label_set_text(city_label, "  Location");
    lv_obj_set_style_text_font(city_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(city_label, lv_color_make(0xAA, 0xAA, 0xAA), LV_PART_MAIN);

    setting_city_label = lv_label_create(city_row);
    lv_label_set_text(setting_city_label, "Hengyang");
    lv_obj_set_style_text_font(setting_city_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(setting_city_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(setting_city_label, SCREEN_WIDTH - 85, 12);

    lv_obj_t *device_row = lv_obj_create(info_card);
    lv_obj_set_size(device_row, SCREEN_WIDTH - 32, 40);
    lv_obj_set_pos(device_row, 8, 52);
    lv_obj_set_style_bg_opa(device_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(device_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(device_row, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(device_row, lv_color_make(0x55, 0x55, 0x55), LV_PART_MAIN);
    lv_obj_set_style_border_width(device_row, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(device_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(device_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(device_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(device_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *device_icon = lv_label_create(device_row);
    lv_label_set_text(device_icon, LV_SYMBOL_DRIVE);
    lv_obj_set_style_text_font(device_icon, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(device_icon, lv_color_make(0x81, 0xC7, 0x84), LV_PART_MAIN);

    lv_obj_t *device_label = lv_label_create(device_row);
    lv_label_set_text(device_label, "  MCU");
    lv_obj_set_style_text_font(device_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(device_label, lv_color_make(0xAA, 0xAA, 0xAA), LV_PART_MAIN);

    lv_obj_t *device_val = lv_label_create(device_row);
    lv_label_set_text(device_val, "STM32F407ZGT6");
    lv_obj_set_style_text_font(device_val, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(device_val, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(device_val, SCREEN_WIDTH - 95, 12);

    lv_obj_t *wifi_row = lv_obj_create(info_card);
    lv_obj_set_size(wifi_row, SCREEN_WIDTH - 32, 40);
    lv_obj_set_pos(wifi_row, 8, 96);
    lv_obj_set_style_bg_opa(wifi_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_row, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(wifi_row, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(wifi_row, lv_color_make(0x55, 0x55, 0x55), LV_PART_MAIN);
    lv_obj_set_style_border_width(wifi_row, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifi_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(wifi_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifi_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(wifi_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *wifi_icon = lv_label_create(wifi_row);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(wifi_icon, lv_color_make(0xFF, 0xB7, 0x4D), LV_PART_MAIN);

    lv_obj_t *wifi_label = lv_label_create(wifi_row);
    lv_label_set_text(wifi_label, "  WiFi Module");
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(wifi_label, lv_color_make(0xAA, 0xAA, 0xAA), LV_PART_MAIN);

    lv_obj_t *wifi_val = lv_label_create(wifi_row);
    lv_label_set_text(wifi_val, "ESP32C3");
    lv_obj_set_style_text_font(wifi_val, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(wifi_val, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_pos(wifi_val, SCREEN_WIDTH - 85, 12);

    lv_obj_t *version_row = lv_obj_create(info_card);
    lv_obj_set_size(version_row, SCREEN_WIDTH - 32, 40);
    lv_obj_set_pos(version_row, 8, 140);
    lv_obj_set_style_bg_opa(version_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(version_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(version_row, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(version_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(version_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(version_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *version_icon = lv_label_create(version_row);
    lv_label_set_text(version_icon, LV_SYMBOL_LIST);
    lv_obj_set_style_text_font(version_icon, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(version_icon, lv_color_make(0xCE, 0x93, 0xD8), LV_PART_MAIN);

    lv_obj_t *version_label = lv_label_create(version_row);
    lv_label_set_text(version_label, "  Firmware");
    lv_obj_set_style_text_font(version_label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(version_label, lv_color_make(0xAA, 0xAA, 0xAA), LV_PART_MAIN);

    setting_version_label = lv_label_create(version_row);
    lv_label_set_text(setting_version_label, "v1.2.0");
    lv_obj_set_style_text_font(setting_version_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(setting_version_label, lv_color_make(0x90, 0xCA, 0xF9), LV_PART_MAIN);
    lv_obj_set_pos(setting_version_label, SCREEN_WIDTH - 75, 12);

    lv_obj_t *hint = lv_label_create(settings_page);
    lv_label_set_text(hint, LV_SYMBOL_LEFT " Swipe to switch " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(hint, lv_color_make(0x88, 0x88, 0x88), LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);
}

/**
 * @brief 在屏幕底部创建三个小圆点，用于指示当前处于哪一个页面
 */
static void create_indicator_bar(void)
{
    // 创建一个承载三个小圆点的长条形容器
    lv_obj_t *indic = lv_obj_create(lv_scr_act());
    lv_obj_set_size(indic, 60, 10);
    lv_obj_set_pos(indic, (SCREEN_WIDTH - 60) / 2, SCREEN_HEIGHT - INDICATOR_HEIGHT / 2 - 5);
    // 使用Flex布局均匀分布三个点
    lv_obj_set_flex_flow(indic, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(indic, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER); 
    
    // 循环创建三个小圆点
    for (int i = 0; i < 3; i++) {
        page_indicator[i] = lv_obj_create(indic);
        lv_obj_set_size(page_indicator[i], 8, 8);
        // 第一页的高亮，其余暗灰
        lv_obj_set_style_bg_color(page_indicator[i], i == 0 ? COLOR_ACCENT : COLOR_INDICATOR_DIM, LV_PART_MAIN);
        lv_obj_set_style_radius(page_indicator[i], 4, LV_PART_MAIN); // 宽高的一半即为圆形
        lv_obj_set_style_opa(page_indicator[i], i == 0 ? 255 : 150, LV_PART_MAIN);
    }
}

/*============================================================================*/
/* 数据更新接口 (供系统业务逻辑层/数据处理任务调用，线程安全)           */
/*============================================================================*/

/**
 * @brief 更新顶部状态栏的WiFi连接状态及SSID名称
 */
void main_page_redraw_wifi_ssid(const char *ssid, bool connected)
{
    lvgl_lock(); // 因为是从后台任务调用，必须加锁保证LVGL不崩溃
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

/**
 * @brief 刷新时分秒
 */
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

/**
 * @brief 刷新年月日及星期
 */
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

/**
 * @brief 刷新室内温度显示 (通常来源于本地DHT11或AHT20传感器)
 */
void main_page_redraw_inner_temperature(float temperature)
{
    lvgl_lock();
    if (indoor_temp_label) {
        char str[16];
        int temp_int = (int)temperature;
        if (temp_int >= 0 && temp_int <= 100) {
            snprintf(str, sizeof(str), "%d", temp_int);
        } else {
            snprintf(str, sizeof(str), "--"); // 容错处理：断开或异常时显示横线
        }
        lv_label_set_text(indoor_temp_label, str);
    }
    lvgl_unlock();
}

/**
 * @brief 刷新室内湿度显示
 */
void main_page_redraw_inner_humidity(float humidity)
{
    lvgl_lock();
    if (indoor_humidity_label) {
        char str[16];
        int humid_int = (int)humidity;
        if (humid_int >= 0 && humid_int <= 100) {
            snprintf(str, sizeof(str), "%d", humid_int);
        } else {
            snprintf(str, sizeof(str), "--");
        }
        lv_label_set_text(indoor_humidity_label, str);
    }
    lvgl_unlock();
}

/**
 * @brief 更新室外定位城市名称 (联动更新多个页面的城市显示)
 */
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

/**
 * @brief 刷新室外温度显示 (来源于网络API，同时更新卡片页和详情页)
 */
void main_page_redraw_outdoor_temperature(float temperature)
{
    lvgl_lock();
    if (outdoor_temp_label) {
        char str[16];
        if (temperature > -10.0f && temperature <= 100.0f) {
            snprintf(str, sizeof(str), "%d", (int)temperature);
        } else {
            snprintf(str, sizeof(str), "--");
        }
        lv_label_set_text(outdoor_temp_label, str);
    }
    // 同步更新第二页超大温度
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

/**
 * @brief 根据获取到的气象代码，更新文字描述和重绘画布图标
 */
void main_page_redraw_outdoor_weather_icon(const int code)
{
    lvgl_lock();
    
    current_weather_code = code;
    
    const char *desc_text = "Unknown";
    
    // 解析气象代码 -> 英语短描述
    if (code == 0 || code == 2 || code == 38) {
        desc_text = "Sunny";
    } else if (code == 1 || code == 3) {
        desc_text = "Clear";
    } else if (code == 4 || code == 9 || code == 30) {
        desc_text = "Cloudy";
    } else if (code == 5 || code == 6 || code == 7 || code == 8) {
        desc_text = "Partly Cloudy";
    } else if (code >= 10 && code <= 19) {
        desc_text = "Rain";
    } else if (code == 11 || code == 12) {
        desc_text = "Thunder";
    } else if (code >= 20 && code <= 25) {
        desc_text = "Snow";
    }
    
    // 调用之前定义的函数重绘Canvas内的图标
    draw_weather_icon(code);
    
    if (weather_icon_label) {
        lv_label_set_text(weather_icon_label, desc_text);
    }
    
    if (weather_desc_label) {
        lv_label_set_text(weather_desc_label, desc_text);
    }
    
    lvgl_unlock();
}

/**
 * @brief 刷新室外湿度(详情页)
 */
void main_page_redraw_outdoor_humidity(int humidity)
{
    lvgl_lock();
    if (weather_humidity_label) {
        char str[32];
        if (humidity >= 0 && humidity <= 100) {
            snprintf(str, sizeof(str), "%d%%", humidity);
        } else {
            snprintf(str, sizeof(str), "--%%");
        }
        lv_label_set_text(weather_humidity_label, str);
    }
    lvgl_unlock();
}

/**
 * @brief 刷新室外风力风向(详情页)
 */
void main_page_redraw_outdoor_wind(int wind_speed, const char *wind_direction)
{
    lvgl_lock();
    if (weather_wind_label) {
        char str[32];
        if (wind_speed >= 0 && wind_direction && strlen(wind_direction) > 0) {
            snprintf(str, sizeof(str), "%s %d km/h", wind_direction, wind_speed);
        } else if (wind_speed >= 0) {
            snprintf(str, sizeof(str), "%d km/h", wind_speed);
        } else {
            snprintf(str, sizeof(str), "-- km/h");
        }
        lv_label_set_text(weather_wind_label, str);
    }
    lvgl_unlock();
}
