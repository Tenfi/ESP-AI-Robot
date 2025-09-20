#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "esp_http_client.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"

#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"

#include "lvgl.h"
#include "iot_button.h"

#include "wifi_sntp.h"
#include "esp_sparkbot_bsp.h"
#include "bsp_board_extra.h"

#include "baidu.h"

#include "cJSON.h"
#include "dirent.h"

#include "ui.h"
#include "app_imu.h"
#include "app_wifi.h"
#include "app_weather.h"
#include "app_audio_record.h"
#include "app_animation.h"
#include "app_power.h"

#include "esp_camera.h"

static const char *TAG = "main";
static void button_handler(touch_button_handle_t out_handle, touch_button_message_t *out_message, void *arg)
{
    (void) out_handle; //Unused
    lv_obj_t *current_screen = lv_disp_get_scr_act(NULL);
    int button = (int)arg;

    if (out_message->event == TOUCH_BUTTON_EVT_ON_PRESS) {
        // ESP_LOGI(TAG, "Button[%d] Press", (int)arg);
        for (int i = 0; i < sizeof(ui_pages) / sizeof(ui_page_name_t); i++) {
            if (ui_pages[i].page == current_screen) {
                printf("current screen is %s\n", ui_pages[i].name);
                break;
            }
        }
        switch (button) {
        case 1:
            ui_send_sys_event(current_screen, LV_EVENT_PRESSED, NULL);
            break;
        case 2:
            ui_send_sys_event(current_screen, LV_EVENT_SCREEN_PRIVIOUS, NULL);
            break;
        case 3:
            ui_send_sys_event(current_screen, LV_EVENT_SCREEN_NEXT, NULL);
            break;
        default:
            break;
        }
    } else if (out_message->event == TOUCH_BUTTON_EVT_ON_RELEASE) {
        // ESP_LOGI(TAG, "Button[%d] Release", (int)arg);
    } else if (out_message->event == TOUCH_BUTTON_EVT_ON_LONGPRESS) {
        // ESP_LOGI(TAG, "Button[%d] LongPress", (int)arg);
        switch (button) {
        case 1:
            ui_send_sys_event(current_screen, LV_EVENT_LONG_PRESSED, NULL);
            break;
        default:
            break;
        }
    }
}

static void button_long_press_cb(void *arg, void *usr_data)
{
    ESP_LOGI(TAG, "BUTTON_LONG_PRESS_START");
    nvs_flash_erase();
    esp_restart();
}

void memory_monitor()
{
    static char buffer[128];    /* Make sure buffer is enough for `sprintf` */
    if (1) {
        sprintf(buffer, "   Biggest /     Free /    Total\n"
                "\t  SRAM : [%8d / %8d / %8d]\n"
                "\t PSRAM : [%8d / %8d / %8d]",
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                heap_caps_get_total_size(MALLOC_CAP_INTERNAL),
                heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
                heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
        ESP_LOGI("MEM", "%s", buffer);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    /* Initialize the power adc */
    power_adc_init();
    bsp_i2c_init();

    /**
     * @brief Connect to the network
     */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    app_animation_start();
    app_imu_init();
    bsp_touch_button_create(button_handler);

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

    ESP_ERROR_CHECK(example_connect());
    wifi_sntp_start();
    ESP_ERROR_CHECK(app_sr_start());

    app_weather_start();
    app_network_start();
    send_network_event(NET_EVENT_WEATHER);

    /* Monitor free heap */
    memory_monitor();
}
