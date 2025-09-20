/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "motion_control.h"

#define LEDC_TIMER                  LEDC_TIMER_0
#define LEDC_MODE                   LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES               LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_FREQUENCY              (4000) // Frequency in Hertz. Set frequency at 4 kHz

#define LEDC_CHANNEL_COUNT          (4)
#define LEDC_M1_CHANNEL_A           LEDC_CHANNEL_0
#define LEDC_M1_CHANNEL_B           LEDC_CHANNEL_1
#define LEDC_M2_CHANNEL_A           LEDC_CHANNEL_2
#define LEDC_M2_CHANNEL_B           LEDC_CHANNEL_3

#define LEDC_M1_CHANNEL_A_IO        (4)
#define LEDC_M1_CHANNEL_B_IO        (5)
#define LEDC_M2_CHANNEL_A_IO        (6)
#define LEDC_M2_CHANNEL_B_IO        (7)

float m1_coefficient = 1.0;
float m2_coefficient = 1.0;

void motor_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Array of channel configurations for easy iteration
    const uint8_t motor_ledc_channel[LEDC_CHANNEL_COUNT] = {LEDC_M1_CHANNEL_A, LEDC_M1_CHANNEL_B, LEDC_M2_CHANNEL_A, LEDC_M2_CHANNEL_B};
    const int32_t ledc_channel_pins[LEDC_CHANNEL_COUNT] = {LEDC_M1_CHANNEL_A_IO, LEDC_M1_CHANNEL_B_IO, LEDC_M2_CHANNEL_A_IO, LEDC_M2_CHANNEL_B_IO};
    for (int i = 0; i < LEDC_CHANNEL_COUNT; i++) {
        ledc_channel_config_t ledc_channel = {
            .speed_mode     = LEDC_MODE,
            .channel        = motor_ledc_channel[i], // Assuming channel values increment for each new channel
            .timer_sel      = LEDC_TIMER,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = ledc_channel_pins[i],
            .duty           = 0, // Set duty to 0%
            .hpoint         = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }
}

static void set_left_motor_speed(int speed)
{
    if (speed >= 0) {
        uint32_t m1a_duty = (uint32_t)((speed * m1_coefficient * 8192) / 100);
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M1_CHANNEL_A, m1a_duty));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M1_CHANNEL_A));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M1_CHANNEL_B, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M1_CHANNEL_B));
    } else {
        uint32_t m1b_duty = (uint32_t)((-speed * m1_coefficient * 8192) / 100);
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M1_CHANNEL_A, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M1_CHANNEL_A));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M1_CHANNEL_B, m1b_duty));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M1_CHANNEL_B));
    }
}

static void set_right_motor_speed(int speed)
{
    if (speed >= 0) {
        uint32_t m2a_duty = (uint32_t)((speed * m2_coefficient * 8192) / 100);
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M2_CHANNEL_A, m2a_duty));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M2_CHANNEL_A));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M2_CHANNEL_B, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M2_CHANNEL_B));
    } else {
        uint32_t m2b_duty = (uint32_t)((-speed * m2_coefficient * 8192) / 100);
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M2_CHANNEL_A, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M2_CHANNEL_A));
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_M2_CHANNEL_B, m2b_duty));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_M2_CHANNEL_B));
    }
}

void spark_bot_motion_control(float x, float y)
{
    float base_speed = y * MOTOR_SPEED_MAX;
    float turn_adjust = x * MOTOR_SPEED_MAX;

    int left_speed = (int)(base_speed + turn_adjust);
    int right_speed = (int)(base_speed - turn_adjust);

    if (left_speed > MOTOR_SPEED_MAX) left_speed = MOTOR_SPEED_MAX;
    if (right_speed > MOTOR_SPEED_MAX) right_speed = MOTOR_SPEED_MAX;

    set_left_motor_speed(left_speed);
    set_right_motor_speed(right_speed);
}

void set_motor_speed_coefficients(float coefficient)
{
    if (coefficient < 0) {
        if (m1_coefficient >= 0.7) {
            m1_coefficient -= 0.025;
        }
        m2_coefficient = 1.0;
    } else {
        if (m2_coefficient >= 0.7) {
            m2_coefficient -= 0.025;
        }
        m1_coefficient = 1.0;
    }
    printf("m1_coefficient = %f, m2_coefficient = %f\n", m1_coefficient, m2_coefficient);
}

typedef struct {
    int         left_speed;
    int         right_speed;
    uint16_t    time;
} dance_motion_t;

void spark_bot_dance(void)
{
    // 使用左右履带速度和时间定义舞蹈动作
    dance_motion_t dance_moves[] = {
        // 第一段舞蹈
        {MOTOR_SPEED_MAX, MOTOR_SPEED_MAX, 1000},  // 向前快速移动 1 秒
        {MOTOR_SPEED_80, -MOTOR_SPEED_80, 800},    // 左转，0.8 秒
        {-MOTOR_SPEED_60, -MOTOR_SPEED_60, 1200},  // 向后慢速移动 1.2 秒
        {-MOTOR_SPEED_80, MOTOR_SPEED_80, 800},    // 右转，0.8 秒
        {MOTOR_SPEED_80, MOTOR_SPEED_80, 1000},    // 向前中速移动 1 秒
        {-MOTOR_SPEED_80, MOTOR_SPEED_80, 800},    // 右转，0.8 秒
        {-MOTOR_SPEED_MAX, -MOTOR_SPEED_MAX, 1000},// 向后快速移动 1 秒
        {MOTOR_SPEED_80, -MOTOR_SPEED_80, 800},    // 左转，0.8 秒
        {0, 0, 500},                               // 停止，0.5 秒

        // 逆向动作，确保回到起点
        {-MOTOR_SPEED_80, MOTOR_SPEED_80, 800},    // 右转，0.8 秒
        {MOTOR_SPEED_MAX, MOTOR_SPEED_MAX, 1000},  // 向前快速移动 1 秒
        {MOTOR_SPEED_80, -MOTOR_SPEED_80, 800},    // 左转，0.8 秒
        {-MOTOR_SPEED_80, -MOTOR_SPEED_80, 1000},  // 向后中速移动 1 秒
        {MOTOR_SPEED_80, -MOTOR_SPEED_80, 800},    // 左转，0.8 秒
        {MOTOR_SPEED_60, MOTOR_SPEED_60, 1200},    // 向前慢速移动 1.2 秒
        {-MOTOR_SPEED_80, MOTOR_SPEED_80, 800},    // 右转，0.8 秒
        {-MOTOR_SPEED_MAX, -MOTOR_SPEED_MAX, 1000},// 向后快速移动 1 秒
        {0, 0, 500},                               // 停止，0.5 秒
    };

    // 遍历动作列表并执行
    for (int i = 0; i < sizeof(dance_moves) / sizeof(dance_moves[0]); i++) {
        // 控制左右电机速度
        set_left_motor_speed(dance_moves[i].left_speed);
        set_right_motor_speed(dance_moves[i].right_speed);

        // 持续指定时间
        vTaskDelay(dance_moves[i].time / portTICK_PERIOD_MS);

        // 停止电机的PWM输出
        set_left_motor_speed(0);
        set_right_motor_speed(0);
    }
}

