/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#ifndef _APP_IMU_H_
#define _APP_IMU_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float acc_x;
    float acc_y;
    float acc_z;
    float gyr_x;
    float gyr_y;
    float gyr_z;
} bmi270_value_t;

void app_imu_init(void);

#ifdef __cplusplus
}
#endif

#endif
