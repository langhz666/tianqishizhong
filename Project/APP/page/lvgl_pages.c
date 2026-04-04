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

extern osMutexId_t lcd_mutex;

static lv_obj_t *ssid_label = NULL;
static lv_obj_t *time_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *week_label = NULL;
static lv_obj_t *indoor_temp_label = NULL;
static lv_obj_t *indoor_humidity_label = NULL;
static lv_obj_t *outdoor_temp_label = NULL;
static lv_obj_t *weather_icon_label = NULL;
static lv_obj_t *outdoor_city_label = NULL;
static lv_obj_t *wifi_status_label = NULL;

static lv_obj_t *splash_screen = NULL;
static lv_obj_t *splash_title = NULL;
static lv_obj_t *splash_subtitle = NULL;
static lv_obj_t *splash_bar = NULL;
static lv_obj_t *splash_bar_bg = NULL;
static lv_obj_t *splash_status = NULL;
static lv_anim_t splash_anim;

#define COLOR_BG_TIME     lv_color_white()
#define COLOR_BG_INNER    lv_color_make(0x86, 0xD5, 0x00)
#define COLOR_BG_OUTDOOR  lv_color_make(0xFD, 0x29, 0x00)
#define COLOR_TEXT_BLACK  lv_color_black()
#define COLOR_TEXT_GRAY   lv_color_make(0x88, 0x88, 0x88)
#define COLOR_TEXT_WHITE  lv_color_white()
#define COLOR_SPLASH_BG   lv_color_make(0x0A, 0x1A, 0x2A)
#define COLOR_SPLASH_ACCENT lv_color_make(0x00, 0xD4, 0xAA)

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

static void splash_bar_anim_cb(void *var, int32_t v)
{
    lv_obj_t *bar = (lv_obj_t *)var;
    lv_obj_set_width(bar, (v * 180) / 100);
}

static void splash_fade_in_cb(lv_anim_t *a)
{
    (void)a;
}

void splash_screen_start(void)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_SPLASH_BG, LV_PART_MAIN);

    splash_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(splash_screen, 240, 320);
    lv_obj_set_pos(splash_screen, 0, 0);
    lv_obj_set_style_bg_color(splash_screen, COLOR_SPLASH_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(splash_screen, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(splash_screen, 0, LV_PART_MAIN);

    lv_obj_t *logo_circle = lv_obj_create(splash_screen);
    lv_obj_set_size(logo_circle, 80, 80);
    lv_obj_set_pos(logo_circle, 80, 60);
    lv_obj_set_style_bg_color(logo_circle, COLOR_SPLASH_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(logo_circle, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(logo_circle, 40, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(logo_circle, 0, LV_PART_MAIN);

    lv_obj_t *logo_text = lv_label_create(logo_circle);
    lv_label_set_text(logo_text, "W");
    lv_obj_set_style_text_color(logo_text, COLOR_SPLASH_BG, LV_PART_MAIN);
    lv_obj_set_style_text_font(logo_text, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_center(logo_text);

    splash_title = lv_label_create(splash_screen);
    lv_label_set_text(splash_title, "Weather Clock");
    lv_obj_set_style_text_color(splash_title, COLOR_TEXT_WHITE, LV_PART_MAIN);
    lv_obj_set_style_text_font(splash_title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_pos(splash_title, 30, 160);

    splash_subtitle = lv_label_create(splash_screen);
    lv_label_set_text(splash_subtitle, "Smart Weather Station");
    lv_obj_set_style_text_color(splash_subtitle, lv_color_make(0x66, 0x88, 0xAA), LV_PART_MAIN);
    lv_obj_set_style_text_font(splash_subtitle, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_pos(splash_subtitle, 45, 190);

    splash_bar_bg = lv_obj_create(splash_screen);
    lv_obj_set_size(splash_bar_bg, 180, 8);
    lv_obj_set_pos(splash_bar_bg, 30, 240);
    lv_obj_set_style_bg_color(splash_bar_bg, lv_color_make(0x1A, 0x2A, 0x3A), LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_bar_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_bar_bg, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(splash_bar_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(splash_bar_bg, 0, LV_PART_MAIN);

    splash_bar = lv_obj_create(splash_bar_bg);
    lv_obj_set_size(splash_bar, 0, 8);
    lv_obj_set_pos(splash_bar, 0, 0);
    lv_obj_set_style_bg_color(splash_bar, COLOR_SPLASH_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(splash_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(splash_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(splash_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(splash_bar, 0, LV_PART_MAIN);

    splash_status = lv_label_create(splash_screen);
    lv_label_set_text(splash_status, "Initializing...");
    lv_obj_set_style_text_color(splash_status, lv_color_make(0x55, 0x77, 0x99), LV_PART_MAIN);
    lv_obj_set_style_text_font(splash_status, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_pos(splash_status, 30, 260);

    lv_obj_t *version = lv_label_create(splash_screen);
    lv_label_set_text(version, "v1.0.0");
    lv_obj_set_style_text_color(version, lv_color_make(0x44, 0x55, 0x66), LV_PART_MAIN);
    lv_obj_set_style_text_font(version, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_pos(version, 185, 295);

    lv_anim_init(&splash_anim);
    lv_anim_set_var(&splash_anim, splash_bar);
    lv_anim_set_exec_cb(&splash_anim, splash_bar_anim_cb);
    lv_anim_set_values(&splash_anim, 0, 100);
    lv_anim_set_time(&splash_anim, 2000);
    lv_anim_set_delay(&splash_anim, 500);
    lv_anim_set_ready_cb(&splash_anim, splash_fade_in_cb);
    lv_anim_start(&splash_anim);

    lvgl_unlock();
}

void splash_set_progress(uint8_t progress, const char *status)
{
    lvgl_lock();
    
    if (splash_bar && progress <= 100)
    {
        lv_obj_set_width(splash_bar, (progress * 180) / 100);
    }
    
    if (splash_status && status)
    {
        lv_label_set_text(splash_status, status);
    }
    
    lvgl_unlock();
}

void splash_screen_end(void)
{
    lvgl_lock();
    
    if (splash_screen)
    {
        lv_obj_del(splash_screen);
        splash_screen = NULL;
        splash_title = NULL;
        splash_subtitle = NULL;
        splash_bar = NULL;
        splash_bar_bg = NULL;
        splash_status = NULL;
    }
    
    lvgl_unlock();
}

void welcome_page_display(void)
{
    splash_screen_start();
}

void error_page_display(const char *msg)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);

    lv_obj_t *icon = lv_label_create(lv_scr_act());
    lv_label_set_text(icon, "!");
    lv_obj_set_style_text_color(icon, lv_color_make(0xFF, 0x44, 0x44), LV_PART_MAIN);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, msg);
    lv_obj_set_style_text_color(label, lv_color_make(0xFF, 0xAA, 0x00), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_width(label, 200);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

    lvgl_unlock();
}

void wifi_page_display(void)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);

    lv_obj_t *icon = lv_label_create(lv_scr_act());
    lv_label_set_text(icon, "WiFi");
    lv_obj_set_style_text_color(icon, lv_color_make(0x07, 0xFD, 0x00), LV_PART_MAIN);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_CENTER, 0, -50);

    ssid_label = lv_label_create(lv_scr_act());
    lv_label_set_text(ssid_label, WIFI_SSID);
    lv_obj_set_style_text_color(ssid_label, COLOR_TEXT_WHITE, LV_PART_MAIN);
    lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(ssid_label, LV_ALIGN_CENTER, 0, 0);

    wifi_status_label = lv_label_create(lv_scr_act());
    lv_label_set_text(wifi_status_label, "Connecting...");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_make(0x96, 0x5F, 0x00), LV_PART_MAIN);
    lv_obj_set_style_text_font(wifi_status_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(wifi_status_label, LV_ALIGN_CENTER, 0, 50);

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

void main_page_display(void)
{
    lvgl_lock();

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_PART_MAIN);

    lv_obj_t *main_time_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_time_bg, 210, 140);
    lv_obj_set_pos(main_time_bg, 15, 15);
    lv_obj_set_style_bg_color(main_time_bg, COLOR_BG_TIME, LV_PART_MAIN);
    lv_obj_set_style_border_width(main_time_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(main_time_bg, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(main_time_bg, 0, LV_PART_MAIN);

    ssid_label = lv_label_create(main_time_bg);
    lv_label_set_text(ssid_label, WIFI_SSID);
    lv_obj_set_style_text_color(ssid_label, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_set_style_text_font(ssid_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_pos(ssid_label, 8, 8);

    time_label = lv_label_create(main_time_bg);
    lv_label_set_text(time_label, "--:--:--");
    lv_obj_set_style_text_color(time_label, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_pos(time_label, 15, 35);

    date_label = lv_label_create(main_time_bg);
    lv_label_set_text(date_label, "----/--/--");
    lv_obj_set_style_text_color(date_label, COLOR_TEXT_GRAY, LV_PART_MAIN);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_pos(date_label, 15, 90);

    week_label = lv_label_create(main_time_bg);
    lv_label_set_text(week_label, "");
    lv_obj_set_style_text_color(week_label, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(week_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_pos(week_label, 140, 90);

    lv_obj_t *main_indoor_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_indoor_bg, 100, 140);
    lv_obj_set_pos(main_indoor_bg, 15, 165);
    lv_obj_set_style_bg_color(main_indoor_bg, COLOR_BG_INNER, LV_PART_MAIN);
    lv_obj_set_style_border_width(main_indoor_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(main_indoor_bg, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(main_indoor_bg, 0, LV_PART_MAIN);

    lv_obj_t *indoor_title = lv_label_create(main_indoor_bg);
    lv_label_set_text(indoor_title, "Indoor");
    lv_obj_set_style_text_color(indoor_title, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(indoor_title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_pos(indoor_title, 10, 5);

    indoor_temp_label = lv_label_create(main_indoor_bg);
    lv_label_set_text(indoor_temp_label, "--");
    lv_obj_set_style_text_color(indoor_temp_label, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(indoor_temp_label, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_pos(indoor_temp_label, 10, 28);

    lv_obj_t *temp_unit = lv_label_create(main_indoor_bg);
    lv_label_set_text(temp_unit, "C");
    lv_obj_set_style_text_color(temp_unit, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(temp_unit, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_pos(temp_unit, 72, 34);

    lv_obj_t *humid_icon = lv_label_create(main_indoor_bg);
    lv_label_set_text(humid_icon, "H");
    lv_obj_set_style_text_color(humid_icon, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(humid_icon, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_pos(humid_icon, 10, 72);

    indoor_humidity_label = lv_label_create(main_indoor_bg);
    lv_label_set_text(indoor_humidity_label, "--");
    lv_obj_set_style_text_color(indoor_humidity_label, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(indoor_humidity_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_pos(indoor_humidity_label, 25, 68);

    lv_obj_t *humid_unit = lv_label_create(main_indoor_bg);
    lv_label_set_text(humid_unit, "%");
    lv_obj_set_style_text_color(humid_unit, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(humid_unit, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_pos(humid_unit, 72, 74);

    lv_obj_t *main_outdoor_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_outdoor_bg, 100, 140);
    lv_obj_set_pos(main_outdoor_bg, 125, 165);
    lv_obj_set_style_bg_color(main_outdoor_bg, COLOR_BG_OUTDOOR, LV_PART_MAIN);
    lv_obj_set_style_border_width(main_outdoor_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(main_outdoor_bg, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(main_outdoor_bg, 0, LV_PART_MAIN);

    outdoor_city_label = lv_label_create(main_outdoor_bg);
    lv_label_set_text(outdoor_city_label, "Outdoor");
    lv_obj_set_style_text_color(outdoor_city_label, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(outdoor_city_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_pos(outdoor_city_label, 10, 5);

    outdoor_temp_label = lv_label_create(main_outdoor_bg);
    lv_label_set_text(outdoor_temp_label, "--");
    lv_obj_set_style_text_color(outdoor_temp_label, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(outdoor_temp_label, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_pos(outdoor_temp_label, 10, 28);

    lv_obj_t *out_temp_unit = lv_label_create(main_outdoor_bg);
    lv_label_set_text(out_temp_unit, "C");
    lv_obj_set_style_text_color(out_temp_unit, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(out_temp_unit, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_pos(out_temp_unit, 72, 34);

    weather_icon_label = lv_label_create(main_outdoor_bg);
    lv_label_set_text(weather_icon_label, "--");
    lv_obj_set_style_text_color(weather_icon_label, COLOR_TEXT_BLACK, LV_PART_MAIN);
    lv_obj_set_style_text_font(weather_icon_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_pos(weather_icon_label, 10, 72);
    lv_obj_set_width(weather_icon_label, 80);
    lv_label_set_long_mode(weather_icon_label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    lvgl_unlock();
}

void main_page_redraw_wifi_ssid(const char *ssid)
{
    lvgl_lock();
    if (ssid_label) {
        lv_label_set_text(ssid_label, ssid);
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
        char str_date[16];
        snprintf(str_date, sizeof(str_date), "%04u/%02u/%02u", date->year, date->month, date->day);
        lv_label_set_text(date_label, str_date);
    }
    if (week_label) {
        const char *week_str[] = {"", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        if (date->weekday >= 1 && date->weekday <= 7) {
            lv_label_set_text(week_label, week_str[date->weekday]);
        }
    }
    lvgl_unlock();
}

void main_page_redraw_inner_temperature(float temperature)
{
    lvgl_lock();
    if (indoor_temp_label) {
        char str[8];
        int temp_int = (int)temperature;
        if (temp_int >= 0 && temp_int <= 100) {
            snprintf(str, sizeof(str), "%2d", temp_int);
        } else {
            snprintf(str, sizeof(str), "--");
        }
        lv_label_set_text(indoor_temp_label, str);
    }
    lvgl_unlock();
}

void main_page_redraw_inner_humidity(float humidity)
{
    lvgl_lock();
    if (indoor_humidity_label) {
        char str[8];
        int humid_int = (int)humidity;
        if (humid_int >= 0 && humid_int <= 100) {
            snprintf(str, sizeof(str), "%2d", humid_int);
        } else {
            snprintf(str, sizeof(str), "--");
        }
        lv_label_set_text(indoor_humidity_label, str);
    }
    lvgl_unlock();
}

void main_page_redraw_outdoor_city(const char *city)
{
    lvgl_lock();
    if (outdoor_city_label) {
        lv_label_set_text(outdoor_city_label, city);
    }
    lvgl_unlock();
}

void main_page_redraw_outdoor_temperature(float temperature)
{
    lvgl_lock();
    if (outdoor_temp_label) {
        char str[8];
        if (temperature > -10.0f && temperature <= 100.0f) {
            snprintf(str, sizeof(str), "%2d", (int)temperature);
        } else {
            snprintf(str, sizeof(str), "--");
        }
        lv_label_set_text(outdoor_temp_label, str);
    }
    lvgl_unlock();
}

void main_page_redraw_outdoor_weather_icon(const int code)
{
    lvgl_lock();
    if (weather_icon_label) {
        const char *icon;
        if (code == 0 || code == 2 || code == 38) {
            icon = "Sunny";
        } else if (code == 1 || code == 3) {
            icon = "Clear";
        } else if (code == 4 || code == 9 || code == 30) {
            icon = "Cloudy";
        } else if (code == 5 || code == 6 || code == 7 || code == 8) {
            icon = "Partly Cld";
        } else if (code >= 10 && code <= 19) {
            icon = "Rain";
        } else if (code == 11 || code == 12) {
            icon = "Thunder";
        } else if (code >= 20 && code <= 25) {
            icon = "Snow";
        } else {
            icon = "N/A";
        }
        lv_label_set_text(weather_icon_label, icon);
    }
    lvgl_unlock();
}
