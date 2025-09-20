/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

void wifi_init_ap(void);
esp_err_t ws_async_send_image(uint8_t *image_payload, size_t len);
httpd_handle_t start_webserver();