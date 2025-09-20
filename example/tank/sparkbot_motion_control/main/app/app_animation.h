/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// typedef enum {
//     AUDIO_WAKENET_END = 0,
//     AUDIO_WAKENET_START,
//     AUDIO_VAD_END,
//     AUDIO_VAD_START,
//     AUDIO_VAD_WAIT,
// } audio_record_state_t;

esp_err_t app_animation_start(void);

#ifdef __cplusplus
}
#endif