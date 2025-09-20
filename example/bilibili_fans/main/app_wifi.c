/* SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "protocol_examples_common.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "ui.h"
#include "lvgl.h"

#define TARGET_FANS_NUMBER  10000

static const char *TAG = "app_wifi";

static int g_fans_prvs = 0;

static char *app_wifi_get_bilibili_fans(const char *uid)
{
    char *url = NULL;
    char *result = NULL;
    asprintf(&url, "https://api.bilibili.com/x/relation/stat?vmid=%s", uid);
    ESP_ERROR_CHECK_WITHOUT_ABORT(url == NULL);
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 60000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_open(client, 0);
    int content_length = esp_http_client_fetch_headers(client);
    if (esp_http_client_is_chunked_response(client)) {
        esp_http_client_get_chunk_length(client, &content_length);
    }
    ESP_LOGD(TAG, "content_length=%d", content_length);
    if (content_length < 0) {
        ESP_LOGE(TAG, "HTTP client fetch headers failed!");
        goto end;
    }
    result = (char *)malloc(content_length + 1);
    int read = esp_http_client_read_response(client, result, content_length);
    if (read != content_length) {
        ESP_LOGE(TAG, "HTTP_ERROR: read=%d, length=%d", read, content_length);
        free(result);
        result = NULL;
    } else {
        result[content_length] = 0;
        ESP_LOGD(TAG, "result: %s, size: %d", result, strlen(result));
    }

end:
    free(url);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return result != NULL ? result : NULL;
}

static int cJSON_parse_follower(const char *json_data)
{
    cJSON* root = cJSON_Parse(json_data);
    if (root == NULL) {
        printf("Error parsing JSON data.\n");
        return NULL;
    }
    cJSON* data = cJSON_GetObjectItem(root, "data");
    if (data != NULL) {
        cJSON* follower = cJSON_GetObjectItem(data, "follower");
        if (follower != NULL) {
            cJSON_Delete(root);
            return follower->valueint;
        } else {
            printf("Error extracting 'follower' parameter.\n");
        }
    } else {
        printf("Error extracting 'data' object.\n");
    }
    cJSON_Delete(root);
    return NULL;
}
#include "esp_sparkbot_bsp.h"
#include "iot_button.h"

extern void button_handler(touch_button_handle_t out_handle, touch_button_message_t *out_message, void *arg);

static void http_get_task(void *pvParameters)
{
    char *text = NULL;
    touch_button_message_t msg = {0};
    while (1) {
        const char *result = app_wifi_get_bilibili_fans(CONFIG_BILIBILI_ACCOUNT_NUMBER);
        if (result != NULL) {
            int fans = cJSON_parse_follower(result);
            asprintf(&text, "祝贺您获得 %d 粉丝", fans);
            lv_label_set_text(ui_LabelFansCount, text);
            free(text);
            if (fans > g_fans_prvs) {
                ESP_LOGI(TAG, "More fans!!!");
                msg.event = TOUCH_BUTTON_EVT_ON_PRESS;
                button_handler(NULL, &msg, (void*)1);
            }
            // if (fans >= TARGET_FANS_NUMBER && fans <= TARGET_FANS_NUMBER + 2) {
            //     msg.event = TOUCH_BUTTON_EVT_ON_LONGPRESS;
            //     button_handler(NULL, &msg, (void*)1);
            // }
            g_fans_prvs = fans;
        }
        /* Refresh every 2 seconds */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void app_wifi_init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Connected to AP, begin http example");

    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}