/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once
#include <esp_err.h>
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool need_hint;
    bool sr_enable;
    bool demo_gui;

    char ssid[32];      /**< SSID of target AP. */
    char password[64];  /**< Password of target AP. */
    uint8_t ssid_len;      /**< SSID of target AP. */
    uint8_t password_len;  /**< Password of target AP. */
} sys_param_t;

typedef enum {
    NET_EVENT_NONE = 0,
    NET_EVENT_RECONNECT,
    NET_EVENT_SCAN,
    NET_EVENT_NTP,
    NET_EVENT_WEATHER,
    NET_EVENT_POWERON_SCAN,
    NET_EVENT_MAX,
} net_event_t;

typedef enum {
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED_OK,
    WIFI_STATUS_CONNECTED_FAILED,
} WiFi_Connect_Status;

void app_network_start(void);
esp_err_t send_network_event(net_event_t event);
WiFi_Connect_Status wifi_connected_already(void);


#ifdef __cplusplus
}
#endif
