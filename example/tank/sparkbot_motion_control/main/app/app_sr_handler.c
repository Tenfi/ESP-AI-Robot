/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_check.h"
#include "app_sr.h"
#include "app_sr_handler.h"
#include "esp_afe_sr_iface.h"
#include "tracked_chassis_control.h"
#include "ui.h"

static const char *TAG = "app_sr_handler";

void sr_handler_task(void *pvParam)
{
    tracked_chassis_control_start();
    QueueHandle_t xQueue = (QueueHandle_t) pvParam;

    while (true) {
        sr_result_t result;
        xQueueReceive(xQueue, &result, portMAX_DELAY);

        ESP_LOGI(TAG, "cmd:%d, wakemode:%d,state:%d", result.command_id, result.wakenet_mode, result.state);

        if (ESP_MN_STATE_TIMEOUT == result.state) {
            ESP_LOGI(TAG, "timeout");
            ui_send_sys_event(ui_face, LV_EVENT_FACE_LOOK, NULL);
            tracked_chassis_rgb_light_control(2);
            continue;
        }

        if (WAKENET_DETECTED == result.wakenet_mode) {
            ESP_LOGI(TAG, "wakenet detected");
            ui_send_sys_event(ui_face, LV_EVENT_FACE_ASK, NULL);
            tracked_chassis_rgb_light_control(3);
            continue;
        }

        if (ESP_MN_STATE_DETECTED & result.state) {
            ESP_LOGI(TAG, "mn detected");

            switch (result.command_id) {
            case 0:
                ESP_LOGI(TAG, "OK, MOVE FORWARD");
                ui_send_sys_event(ui_face, LV_EVENT_FACE_RUNNING, NULL);
                tracked_chassis_motion_control("x0.0y1.0");
                vTaskDelay(500 / portTICK_PERIOD_MS);
                tracked_chassis_motion_control("x0.0y0.0");
                ui_send_sys_event(ui_face, LV_EVENT_FACE_LOOK, NULL);
                break;
            case 1:
                ESP_LOGI(TAG, "OK, MOVE BACKWARD");
                ui_send_sys_event(ui_face, LV_EVENT_FACE_RUNNING, NULL);
                tracked_chassis_motion_control("x0.0y-1.0");
                vTaskDelay(500 / portTICK_PERIOD_MS);
                tracked_chassis_motion_control("x0.0y0.0");
                ui_send_sys_event(ui_face, LV_EVENT_FACE_LOOK, NULL);
                break;
            case 2:
                ESP_LOGI(TAG, "OK, TURN LEFT");
                ui_send_sys_event(ui_face, LV_EVENT_FACE_RUNNING, NULL);
                tracked_chassis_motion_control("x-1.0y0.0");
                vTaskDelay(600 / portTICK_PERIOD_MS);
                tracked_chassis_motion_control("x0.0y0.0");
                ui_send_sys_event(ui_face, LV_EVENT_FACE_LOOK, NULL);
                break;
            case 3:
                ESP_LOGI(TAG, "OK, TURN RIGHT");
                ui_send_sys_event(ui_face, LV_EVENT_FACE_RUNNING, NULL);
                tracked_chassis_motion_control("x1.0y0.0");
                vTaskDelay(600 / portTICK_PERIOD_MS);
                tracked_chassis_motion_control("x0.0y0.0");
                ui_send_sys_event(ui_face, LV_EVENT_FACE_LOOK, NULL);
                break;
            case 4:
                tracked_chassis_rgb_light_control(3);
                break;
            case 5:
                tracked_chassis_rgb_light_control(5);
                break;
            case 6:
                tracked_chassis_rgb_light_control(2);
                break;
            case 7:
                tracked_chassis_rgb_light_control(7);
                break;
            case 8:
                tracked_chassis_set_dance_mode(1);
                break;
            default:
                break;
            }
            /* **************** REGISTER COMMAND CALLBACK HERE **************** */
        }
    }

    vTaskDelete(NULL);
}