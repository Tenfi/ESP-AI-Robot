/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sparkbot_bsp.h"
#include "bsp_board_extra.h"
#include "iot_button.h"
#include "mmap_generate_audio.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "app_wifi.h"

#define TAG "ESP-EXAMPLE"

typedef struct {
    size_t len;
    uint8_t *wav;
} audio_data_t;

static mmap_assets_handle_t     asset_audio;
static esp_codec_dev_handle_t   spk_codec_dev       = NULL;
static QueueHandle_t            g_queue_audio_play  = NULL;

/*******************************************************************************
* Private functions
*******************************************************************************/

esp_err_t audio_play(uint8_t *wav, size_t len)
{
    audio_data_t audio_data = {
        .len = len,
        .wav = heap_caps_malloc(len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
    };

    if (audio_data.wav == NULL) {
        ESP_LOGE(TAG, "heap_caps_malloc failed");
        return ESP_OK;
    }

    memcpy(audio_data.wav, wav, len);

    xQueueSend(g_queue_audio_play, &audio_data, portMAX_DELAY);
    return ESP_OK;
}

void button_handler(touch_button_handle_t out_handle, touch_button_message_t *out_message, void *arg)
{
    (void) out_handle; //Unused
    lv_obj_t *current_screen = lv_disp_get_scr_act(NULL);
    int button = (int)arg;

    if (out_message->event == TOUCH_BUTTON_EVT_ON_PRESS) {
        // ESP_LOGI(TAG, "Button[%d] Press", (int)arg);
        if (button == 1) {
            void *audio = (void *)mmap_assets_get_mem(asset_audio, MMAP_AUDIO_YAY_WAV);
            uint32_t len = mmap_assets_get_size(asset_audio, MMAP_AUDIO_YAY_WAV);
            audio_play(audio, len);
        }
    } else if (out_message->event == TOUCH_BUTTON_EVT_ON_RELEASE) {
        // ESP_LOGI(TAG, "Button[%d] Release", (int)arg);
    } else if (out_message->event == TOUCH_BUTTON_EVT_ON_LONGPRESS) {
        // ESP_LOGI(TAG, "Button[%d] LongPress", (int)arg);
        if (button == 1) {
            void *audio = (void *)mmap_assets_get_mem(asset_audio, MMAP_AUDIO_CLAPPING_WAV);
            uint32_t len = mmap_assets_get_size(asset_audio, MMAP_AUDIO_CLAPPING_WAV);
            audio_play(audio, len);
        }
    }
}

static void button_long_press_cb(void *arg, void *usr_data)
{
    ESP_LOGI(TAG, "BUTTON_LONG_PRESS_START");
    nvs_flash_erase();
    esp_restart();
}

static void mmap_audio_init()
{
    const mmap_assets_config_t config = {
        .partition_label = "storage",
        .max_files = MMAP_AUDIO_FILES,
        .checksum = MMAP_AUDIO_CHECKSUM,
        .flags = {
            .mmap_enable = true,
            .app_bin_check = true,
        },
    };

    mmap_assets_new(&config, &asset_audio);
    ESP_LOGI(TAG, "stored_files:%d", mmap_assets_get_stored_files(asset_audio));
}

static void audio_play_task(void *arg)
{
    spk_codec_dev = bsp_extra_audio_codec_speaker_init();
    esp_codec_dev_set_out_vol(spk_codec_dev, 100);

    esp_codec_dev_sample_info_t fs = {
        .sample_rate        = 16000,
        .channel            = 1,
        .bits_per_sample    = 16,
    };

    esp_codec_dev_open(spk_codec_dev, &fs);
    audio_data_t audio_data = {0};

    while (xQueueReceive(g_queue_audio_play, &audio_data, portMAX_DELAY) == pdTRUE) {
        // ESP_LOGW(TAG, "audio_play_task: %d, wav: %p", audio_data.len, audio_data.wav);
        int res = esp_codec_dev_write(spk_codec_dev, audio_data.wav, audio_data.len);
        // ESP_LOGI(TAG, "esp_codec_dev_write %d", res);
        free(audio_data.wav);
    }

    esp_codec_dev_close(spk_codec_dev);
    vTaskDelete(NULL);
}

// *INDENT-OFF*
void app_lvgl_display(void)
{
    bsp_display_lock(0);

    ui_init();

    bsp_display_unlock();
}

void app_main(void)
{
    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();

    /* Create GPIO button */
    button_config_t gpio_btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = 0,
            .active_level = 0,
        },
    };
    button_handle_t gpio_btn = iot_button_create(&gpio_btn_cfg);

    if (NULL == gpio_btn) {
        ESP_LOGE(TAG, "Button create failed");
    }

    iot_button_register_cb(gpio_btn, BUTTON_LONG_PRESS_START, button_long_press_cb, NULL);

    /* Create touch button */
    bsp_touch_button_create(button_handler);

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

    /* Add and show objects on display */
    app_lvgl_display();

    /* Start audio task */
    g_queue_audio_play = xQueueCreate(1, sizeof(audio_data_t));
    mmap_audio_init();
    xTaskCreate(audio_play_task, "audio_play_task", 1024 * 5, NULL, 15, NULL);

    /* Init network */
    app_wifi_init();

    ESP_LOGI(TAG, "Example initialization done.");
}
// *INDENT-ON*
