#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_camera.h"

#include "lvgl.h"
#include "lv_lottie.h"
#include "ui.h"

#include "esp_sparkbot_bsp.h"
#include "bsp_board_extra.h"

#include "app_sr.h"

#include "app_animation.h"

#include "mmap_generate_lottie_assets.h"

#include "web_server.h"



static const char *TAG = "app_main";

extern mmap_assets_handle_t asset_lottie;

static void button_handler(touch_button_handle_t out_handle, touch_button_message_t *out_message, void *arg)
{
    (void) out_handle; //Unused
    lv_obj_t *current_screen = lv_disp_get_scr_act(NULL);
    int button = (int)arg;
    if (out_message->event == TOUCH_BUTTON_EVT_ON_PRESS) {
        ESP_LOGI(TAG, "Button[%d] Press", (int)arg);

        /* Send event to LVGL */
        for (int i = 0; i < sizeof(ui_pages) / sizeof(ui_page_name_t); i++) {
            if (ui_pages[i].page == current_screen) {
                printf("current screen is %s\n", ui_pages[i].name);
                break;
            }
        }

        switch (button) {
        case 1:
            ui_send_sys_event(current_screen, LV_EVENT_FACE_THINK, NULL);
            break;
        case 2:
            // ui_send_sys_event(current_screen, LV_EVENT_SCREEN_PRIVIOUS, NULL);
            break;
        case 3:
            // ui_send_sys_event(current_screen, LV_EVENT_SCREEN_NEXT, NULL);
            break;
        default:
            break;
        }

    } else if (out_message->event == TOUCH_BUTTON_EVT_ON_RELEASE) {
        ESP_LOGI(TAG, "Button[%d] Release", (int)arg);
        ui_send_sys_event(current_screen, LV_EVENT_FACE_LOOK, NULL);
    } else if (out_message->event == TOUCH_BUTTON_EVT_ON_LONGPRESS) {
        ESP_LOGI(TAG, "Button[%d] LongPress", (int)arg);
    }
}

static void button_long_press_cb(void *arg, void *usr_data)
{
    ESP_LOGI(TAG, "BUTTON_LONG_PRESS_START");
    esp_restart();
}

void app_main(void)
{
    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    bsp_i2c_init();
    app_animation_start();

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

    /* Initialize the camera */
    const camera_config_t camera_config = BSP_CAMERA_DEFAULT_CONFIG;
    err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return;
    }
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, BSP_CAMERA_VFLIP);
    s->set_hmirror(s, BSP_CAMERA_HMIRROR);
    ESP_LOGI(TAG, "Camera Init done");

    /* Create touch button */
    bsp_touch_button_create(button_handler);

    /* Initialize audio */
    ESP_ERROR_CHECK(app_sr_start());

    wifi_init_ap();
    start_webserver();


    ESP_LOGI(TAG, "Free memory after start: %d bytes", heap_caps_get_total_size(MALLOC_CAP_INTERNAL));

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
