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
#include "bsp/esp-bsp.h"

#include "app_animation.h"
#include "ui/ui.h"
#include "thorvg_capi.h"
#include "mmap_generate_lottie_assets.h"

static const char *TAG = "app_animation";

mmap_assets_handle_t asset_lottie;

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
}

static void app_lvgl_display(void)
{
    bsp_display_lock(0);
    printf("LVGL init1\n");

    ui_init();
    printf("LVGL init2\n");

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

    /* Add and show objects on display */
    app_lvgl_display();

    return ESP_OK;
}