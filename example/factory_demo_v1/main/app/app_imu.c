
/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "unity.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "bsp/esp-bsp.h"

#include "app_imu.h"
#include "app_datafusion.h"
#include "bmi270.h"
#include "common/common.h"
#include "ui.h"

static const char *TAG = "imu";

#define GRAVITY_EARTH       (9.80665f)
#define ACCEL               UINT8_C(0x00)
#define GYRO                UINT8_C(0x01)

#define FILTER_ALPHA 0.5f
#define TRIGGER_THRESHOLD      4.5f            //deg
static bool gesright_sentalready = false;
static bool gesleft_sentalready = false;
static bool gesup_sentalready = false;
static bool gesdown_sentalready = false;
static bmi270_handle_t bmi_handle = NULL;
static bmi270_axis_t axis_last_val = {0.0f, 0.0f, 0.0f};

static void i2c_sensor_bmi270_init(void)
{
    i2c_bus_handle_t i2c_bus_handle = bsp_i2c_get_handle();
    if(!i2c_bus_handle){
        ESP_LOGE(TAG, "Failed to get i2c bus handle");
        return;
    }

    bmi270_i2c_config_t i2c_bmi270_conf = {
        .i2c_handle = i2c_bus_handle,
        .i2c_addr = BMI270_I2C_ADDRESS,
    };

    bmi270_sensor_create(&i2c_bmi270_conf, &bmi_handle);
}

static float lsb_to_mps2(int16_t val, float g_range, uint8_t bit_width)
{
    double power = 2;

    float half_scale = (float)((pow((double)power, (double)bit_width) / 2.0f));

    return (GRAVITY_EARTH * val * g_range) / half_scale;
}

static float lsb_to_dps(int16_t val, float dps, uint8_t bit_width)
{
    double power = 2;

    float half_scale = (float)((pow((double)power, (double)bit_width) / 2.0f));

    return (dps / (half_scale)) * (val);
}

bmi270_axis_t applyDiceInertia(float dice_x_set, float dice_y_set, float dice_z_set)
{
    float left;

    static float stop_x = 0;
    static float stop_y = 0;
    static float stop_z = 0;

    static float dice_x_rotation = 0.0f;
    static float dice_y_rotation = 0.0f;
    static float dice_z_rotation = 0.0f;

    static float dice_dst_x = 0.0f;
    static float dice_dst_y = 0.0f;
    static float dice_dst_z = 0.0f;

    dice_x_rotation += dice_x_set;
    dice_y_rotation += dice_y_set;
    dice_z_rotation += dice_z_set;

    // Check if the velocities are close to zero, and stop the rotation
    if (fabs(dice_x_set - stop_x) < 2.5f && fabs(dice_y_set - stop_y) < 2.5f && fabs(dice_z_set - stop_z) < 2.5f) {
        if (fabs(dice_x_set - stop_x) < 2.5f) {
            if (dice_x_set) {
                left = fmod(dice_x_rotation, 90.0f);
                if (dice_x_rotation > 0) {
                    dice_dst_x = fabs(left) < 45 ? dice_x_rotation - left : dice_x_rotation + (90 - fabs(left));
                } else {
                    dice_dst_x = fabs(left) < 45 ? dice_x_rotation + fabs(left) : dice_x_rotation - (90 - fabs(left));
                }
            }
        }

        if (fabs(dice_y_set - stop_y) < 2.5f) {
            if (dice_y_set) {
                left = fmod(dice_y_rotation, 90.0f);
                if (dice_y_rotation > 0) {
                    dice_dst_y = fabs(left) < 45 ? dice_y_rotation - left : dice_y_rotation + (90 - fabs(left));
                } else {
                    dice_dst_y = fabs(left) < 45 ? dice_y_rotation + fabs(left) : dice_y_rotation - (90 - fabs(left));
                }
            }
        }

        if (fabs(dice_z_set - stop_z) < 2.5f) {
            if (dice_z_set) {
                left = fmod(dice_z_rotation, 90.0f);
                if (dice_z_rotation > 0) {
                    dice_dst_z = fabs(left) < 45 ? dice_z_rotation - left : dice_z_rotation + (90 - fabs(left));
                } else {
                    dice_dst_z = fabs(left) < 45 ? dice_z_rotation + fabs(left) : dice_z_rotation - (90 - fabs(left));
                }
            }
        }

        dice_x_set = 0.0f;
        dice_y_set = 0.0f;
        dice_z_set = 0.0f;
        stop_x = 0.0f;
        stop_y = 0.0f;
        stop_z = 0.0f;

    } else {
        stop_x = dice_x_set;
        stop_y = dice_y_set;
        stop_z = dice_z_set;
    }

    if (dice_dst_x) {
        if (fabs(dice_dst_x - dice_x_rotation) < 0.1f) {
            dice_x_rotation = dice_dst_x;
            dice_dst_x = 0;
        } else {
            dice_x_rotation += (dice_dst_x > dice_x_rotation ? 1 : -1) * fmin(8.05f, fabs(dice_dst_x - dice_x_rotation) * 0.2f);
        }
    }

    if (dice_dst_y) {
        if (fabs(dice_dst_y - dice_y_rotation) < 0.1f) {
            dice_y_rotation = dice_dst_y;
            dice_dst_y = 0;
        } else {
            dice_y_rotation += (dice_dst_y > dice_y_rotation ? 1 : -1) * fmin(8.05f, fabs(dice_dst_y - dice_y_rotation) * 0.2f);
        }
    }

    if (dice_dst_z) {
        if (fabs(dice_dst_z - dice_z_rotation) < 0.1f) {
            dice_z_rotation = dice_dst_z;
            dice_dst_z = 0;
        } else {
            dice_z_rotation += (dice_dst_z > dice_z_rotation ? 1 : -1) * fmin(8.05f, fabs(dice_dst_z - dice_z_rotation) * 0.2f);
        }
    }

    bmi270_axis_t dice_axis;
    dice_axis.pitch = dice_x_rotation;
    dice_axis.roll = dice_y_rotation;
    dice_axis.yaw = dice_z_rotation;

    return dice_axis;
}

static int8_t set_accel_gyro_config(struct bmi2_dev *bmi)
{

    int8_t rslt;
    struct bmi2_sens_config config[2];

    config[ACCEL].type = BMI2_ACCEL;
    config[GYRO].type = BMI2_GYRO;

    rslt = bmi2_get_sensor_config(config, 2, bmi);
    bmi2_error_codes_print_result(rslt);

    rslt = bmi2_map_data_int(BMI2_DRDY_INT, BMI2_INT1, bmi);
    bmi2_error_codes_print_result(rslt);

    if (rslt == BMI2_OK) {

        config[ACCEL].cfg.acc.odr = BMI2_ACC_ODR_200HZ;
        config[ACCEL].cfg.acc.range = BMI2_ACC_RANGE_2G;
        config[ACCEL].cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;
        config[ACCEL].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;
        config[GYRO].cfg.gyr.odr = BMI2_GYR_ODR_200HZ;
        config[GYRO].cfg.gyr.range = BMI2_GYR_RANGE_2000;
        config[GYRO].cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;
        config[GYRO].cfg.gyr.noise_perf = BMI2_POWER_OPT_MODE;
        config[GYRO].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;

        rslt = bmi2_set_sensor_config(config, 2, bmi);
        bmi2_error_codes_print_result(rslt);
    }

    return rslt;
}

static void bmi270_enable_accel_gyro(struct bmi2_dev *bmi2_dev)
{
    int8_t rslt;

    /* Assign accel and gyro sensor to variable. */
    uint8_t sensor_list[2] = { BMI2_ACCEL, BMI2_GYRO };

    struct bmi2_sens_config config;
    /* Accel and gyro configuration settings. */
    rslt = set_accel_gyro_config(bmi2_dev);
    bmi2_error_codes_print_result(rslt);

    if (rslt == BMI2_OK) {
        rslt = bmi2_sensor_enable(sensor_list, 2, bmi2_dev);
        bmi2_error_codes_print_result(rslt);

        if (rslt == BMI2_OK) {
            config.type = BMI2_ACCEL;
            rslt = bmi2_get_sensor_config(&config, 1, bmi2_dev);
            bmi2_error_codes_print_result(rslt);
        }
    }
}

static void bmi270_get_accel_gyro_value(struct bmi2_dev *bmi2_dev, bmi270_value_t *accel_gyro_val)
{
    int8_t rslt;
    struct bmi2_sens_data sensor_data;
    rslt = bmi2_get_sensor_data(&sensor_data, bmi2_dev);
    bmi2_error_codes_print_result(rslt);

    if ((rslt == BMI2_OK) && (sensor_data.status & BMI2_DRDY_ACC) &&
            (sensor_data.status & BMI2_DRDY_GYR)) {
        /* Converting lsb to meter per second squared for 16 bit accelerometer at 2G range. */
        accel_gyro_val->acc_x = lsb_to_mps2(sensor_data.acc.x, (float)2, bmi2_dev->resolution);
        accel_gyro_val->acc_y = lsb_to_mps2(sensor_data.acc.y, (float)2, bmi2_dev->resolution);
        accel_gyro_val->acc_z = lsb_to_mps2(sensor_data.acc.z, (float)2, bmi2_dev->resolution);

        /* Converting lsb to degree per second for 16 bit gyro at 2000dps range. */
        accel_gyro_val->gyr_x = lsb_to_dps(sensor_data.gyr.x, (float)2000, bmi2_dev->resolution);
        accel_gyro_val->gyr_y = lsb_to_dps(sensor_data.gyr.y, (float)2000, bmi2_dev->resolution);
        accel_gyro_val->gyr_z = lsb_to_dps(sensor_data.gyr.z, (float)2000, bmi2_dev->resolution);
    }
}

static bmi270_axis_t app_imu_read(void)
{
    bmi270_value_t accel_gyro_val;
    bmi270_axis_t axis_val = {0.0f, 0.0f, 0.0f};
    bmi270_axis_t axis_offset = {0.0f, 0.0f, 0.0f};

    bmi270_get_accel_gyro_value(bmi_handle, &accel_gyro_val);
    calculateAttitude(accel_gyro_val.gyr_x, accel_gyro_val.gyr_y, accel_gyro_val.gyr_z, accel_gyro_val.acc_x, accel_gyro_val.acc_y, accel_gyro_val.acc_z, 0.01f, &axis_val);
    // ESP_LOGI(TAG, "%5.2f, %5.2f, %5.2f", axis_val.pitch, axis_val.yaw, axis_val.roll);

    axis_offset.pitch = axis_val.pitch - axis_last_val.pitch;
    axis_offset.roll = axis_val.roll - axis_last_val.roll;
    axis_offset.yaw = axis_val.yaw - axis_last_val.yaw;

    axis_last_val.pitch = axis_val.pitch;
    axis_last_val.roll = axis_val.roll;
    axis_last_val.yaw = axis_val.yaw;
    return axis_offset;
}

static void app_dice_event(bmi270_axis_t axis_offset)
{
    bmi270_axis_t dice_axis = applyDiceInertia(axis_offset.pitch * 15, axis_offset.roll * 15, axis_offset.yaw * 15);
    bsp_display_lock(0);
    // ESP_LOGI(TAG, "pitch,yaw: %5.2f, %5.2f, %5.2f", axis_offset.pitch, axis_offset.yaw, axis_offset.roll);
    lv_event_send(ui_dice_canvas, LV_EVENT_VALUE_CHANGED, &dice_axis);
    bsp_display_unlock();
}
static void app_game_event(bmi270_axis_t axis_offset)
{
    // ESP_LOGI(TAG, "pitch,yaw: %5.2f, %5.2f", axis_offset.pitch, axis_offset.yaw);
    if ((axis_offset.pitch < -TRIGGER_THRESHOLD) && !gesup_sentalready) {
        uint8_t key = LV_KEY_UP;
        lv_event_send(ui_2048, LV_EVENT_KEY, &key);
        ESP_LOGI(TAG, "key up %5.2f", axis_offset.pitch);
        gesup_sentalready = true;
    } else if ((axis_offset.pitch > TRIGGER_THRESHOLD) && !gesdown_sentalready) {
        uint8_t key = LV_KEY_DOWN;
        lv_event_send(ui_2048, LV_EVENT_KEY, &key);
        ESP_LOGI(TAG, "key down %5.2f", axis_offset.pitch);
        gesdown_sentalready = true;
    } else {
        if (fabs(axis_offset.pitch) < 0.3 * TRIGGER_THRESHOLD) {
            gesdown_sentalready = false;
            gesup_sentalready = false;
        }
    }
    if ((axis_offset.yaw > TRIGGER_THRESHOLD) && !gesright_sentalready) {
        uint8_t key = LV_KEY_RIGHT;
        lv_event_send(ui_2048, LV_EVENT_KEY, &key);
        ESP_LOGI(TAG, "key right %5.2f", axis_offset.yaw);
        gesright_sentalready = true;
    } else if ((axis_offset.yaw < -TRIGGER_THRESHOLD) && !gesleft_sentalready) {
        uint8_t key = LV_KEY_LEFT;
        lv_event_send(ui_2048, LV_EVENT_KEY, &key);
        ESP_LOGI(TAG, "key left %5.2f", axis_offset.yaw);
        gesleft_sentalready = true;
    } else {
        if (fabs(axis_offset.yaw) < 0.3 * TRIGGER_THRESHOLD) {
            gesright_sentalready = false;
            gesleft_sentalready = false;
        }
    }
}

static void app_imu_task(void *arg)
{
    while (1) {
        bmi270_axis_t axis_offset = app_imu_read();
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_obj_t * current_screen = lv_disp_get_scr_act(NULL);
        if (current_screen == ui_dice) {
            app_dice_event(axis_offset);
        } else if (current_screen == ui_game) {
            app_game_event(axis_offset);
        }
    }
    vTaskDelete(NULL);
}

void app_imu_init(void)
{
    i2c_sensor_bmi270_init();
    bmi270_enable_accel_gyro(bmi_handle);
    BaseType_t res;

    res = xTaskCreate(app_imu_task, "imu task", 4 * 1024, NULL, 5, NULL);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "Create imu task fail!");
    }
}