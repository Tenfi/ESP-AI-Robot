/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/i2c.h"

#define BMI270_I2C_ADDRESS         0x68 /*!< I2C address with AD0 pin low */


typedef struct {
    float x;
    float y;
    float z;
    float accx;
    float accy;
    float accz;
} bmi270_value_t;

typedef void *bmi270_handle_t;

esp_err_t bmi270_create(i2c_port_t port, const uint8_t dev_addr, bmi270_handle_t *handle_ret);

/**
 * @brief Delete and release a sensor object
 *
 * @param sensor object handle of bmi270
 */
void bmi270_delete(bmi270_handle_t sensor);

/**
 * @brief Read gyro values
 *
 * @param sensor object handle of bmi270
 * @param value gyroscope measurements
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t bmi270_get_gyro_value(bmi270_handle_t sensor, bmi270_value_t *value);


#ifdef __cplusplus
}
#endif
