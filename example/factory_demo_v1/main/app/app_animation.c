/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lv_decoder.h"
#include "esp_lv_fs.h"
#include "bsp/esp-bsp.h"

#include "app_animation.h"
#include "app_weather.h"
#include "app_power.h"
#include "ui/ui.h"
#include "thorvg_capi.h"
#include "mmap_generate_lottie_assets.h"
#include "mmap_generate_weather.h"

static const char *TAG = "app_animation";

mmap_assets_handle_t asset_lottie;
mmap_assets_handle_t asset_weather;

esp_lv_fs_handle_t fs_drive_handle;
esp_lv_decoder_handle_t decoder_handle = NULL;

static void app_mount_mmap_fs()
{
    const mmap_assets_config_t config_lottie = {
        .partition_label = "animation",
        .max_files = MMAP_LOTTIE_ASSETS_FILES,
        .checksum = MMAP_LOTTIE_ASSETS_CHECKSUM,
        .flags = {
            .mmap_enable = true,
            .app_bin_check = true,
        },
    };

    mmap_assets_new(&config_lottie, &asset_lottie);
    ESP_LOGI(TAG, "[%s]stored_files:%d", config_lottie.partition_label, mmap_assets_get_stored_files(asset_lottie));

    const mmap_assets_config_t config_weather = {
        .partition_label = "weather",
        .max_files = MMAP_WEATHER_FILES,
        .checksum = MMAP_WEATHER_CHECKSUM,
        .flags = {
            .mmap_enable = true,
            .app_bin_check = true,
        },
    };

    mmap_assets_new(&config_weather, &asset_weather);
    ESP_LOGI(TAG, "[%s]stored_files:%d", config_weather.partition_label, mmap_assets_get_stored_files(asset_weather));
}

esp_err_t lv_fs_add(void)
{
    fs_cfg_t fs_cfg;
    fs_cfg.fs_letter = 'A';
    fs_cfg.fs_assets = asset_lottie;
    fs_cfg.fs_nums = MMAP_LOTTIE_ASSETS_FILES;

    esp_err_t ret = esp_lv_fs_desc_init(&fs_cfg, &fs_drive_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize FS");
        return ret;
    }
    return ESP_OK;
}

int find_icon_index(char * data)
{
    int i, j;
    char icon_name [10];
    snprintf(icon_name, sizeof(icon_name), "%s.qoi", data);
    for (i = 0; i < mmap_assets_get_stored_files(asset_weather); i++) {
        if (!strcmp(mmap_assets_get_name(asset_weather, i), icon_name)) {
            break;
        }
    }
    if (i == mmap_assets_get_stored_files(asset_weather)) {
        for (j = 0; i < mmap_assets_get_stored_files(asset_weather); j++) {
            if (!strcmp(mmap_assets_get_name(asset_weather, i), "999.qoi")) {
                break;
            }
        }
        i = j;
    }
    return i;
}

void updata_weather_icon(void)
{
    static lv_img_dsc_t img_weather_dsc;

    img_weather_dsc.data_size = mmap_assets_get_size(asset_weather, find_icon_index(weather_icon));
    img_weather_dsc.data = mmap_assets_get_mem(asset_weather, find_icon_index(weather_icon));
    lv_img_set_src(ui_weathershow, &img_weather_dsc);
}

struct timeval tv_now = {
    .tv_sec = 0,
    .tv_usec = 0
};

void ui_clock_update(lv_timer_t *timer)
{
    struct tm timeinfo;
    static char hour_str[3], min_str[3], time_str[6];
    static char date_str[16], weekday_str[16];
    static char power_state_text[5];

    gettimeofday(&tv_now, NULL);
    localtime_r(&tv_now.tv_sec, &timeinfo);

    //format hour,minute,second
    snprintf(hour_str, sizeof(hour_str), "%02d", timeinfo.tm_hour);
    snprintf(min_str, sizeof(min_str), "%02d", timeinfo.tm_min);
    snprintf(time_str, sizeof(time_str), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    snprintf(power_state_text, sizeof(power_state_text), "%d%%", get_power_value());

    //format month and day(for example:10/18)
    snprintf(date_str, sizeof(date_str), "%02d/%02d",
             timeinfo.tm_mon + 1, timeinfo.tm_mday);

    //get weekday
    const char *weekdays[] = {"周日", "周一", "周二", "周三",
                              "周四", "周五", "周六"
                             };
    snprintf(weekday_str, sizeof(weekday_str), "%s", weekdays[timeinfo.tm_wday]);

    //update text of each label
    lv_label_set_text(ui_hour, hour_str);
    lv_label_set_text(ui_min, min_str);
    lv_label_set_text(ui_date, date_str);
    lv_label_set_text(ui_weekday, weekday_str);
    lv_label_set_text(title_timestate, time_str);

    lv_label_set_text(ui_weather, weather_text);
    lv_label_set_text(ui_temp, weather_temp);
    lv_label_set_text(title_batterytxt, power_state_text);
    lv_slider_set_value(title_powerstate, get_power_value(), LV_ANIM_OFF);
    if (1) {
        // if(wifi_connected_already() == WIFI_STATUS_CONNECTED_OK){
        lv_img_set_src(title_wifistate, &ui_img_wifi_png);
    } else {
        lv_img_set_src(title_wifistate, &ui_img_wifi_disconnection_png);
    }
    updata_weather_icon();
}

void ui_init_timer()
{
    lv_timer_t * timer_clock = lv_timer_create(ui_clock_update, 1000,  NULL);
    ui_clock_update(timer_clock);
}

static void app_lvgl_display(void)
{
    bsp_display_lock(0);

    ui_init();
    ui_init_timer();

    bsp_display_unlock();
}

esp_err_t app_animation_start(void)
{
    /* Initialize display and LVGL */
    bsp_display_cfg_t custom_cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * BSP_LCD_V_RES,
        .trans_size = BSP_LCD_H_RES * 10, // in SRAM, DMA-capable
        .double_buffer = 0,
        .flags = {
            .buff_dma = false,
            .buff_spiram = true,
        }
    };
    custom_cfg.lvgl_port_cfg.task_stack = 1024 * 30;
    custom_cfg.lvgl_port_cfg.task_affinity = 1;
    bsp_display_start_with_config(&custom_cfg);

    /* Turn on display backlight */
    bsp_display_backlight_on();

    app_mount_mmap_fs();

    ESP_ERROR_CHECK(lv_fs_add());

    ESP_ERROR_CHECK(esp_lv_decoder_init(&decoder_handle));

    /* Add and show objects on display */
    app_lvgl_display();

    return ESP_OK;
}