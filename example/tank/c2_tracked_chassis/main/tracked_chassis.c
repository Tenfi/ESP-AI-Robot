#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "light_control.h"
#include "motion_control.h"
#include "power_management.h"

static const char *TAG = "Tracked-Chassis";

#define UART_ECHO_TXD (GPIO_NUM_10)
#define UART_ECHO_RXD (GPIO_NUM_18)
#define UART_ECHO_RTS (-1)
#define UART_ECHO_CTS (-1)

#define ECHO_UART_PORT_NUM      (1)
#define ECHO_UART_BAUD_RATE     (115200)
#define ECHO_TASK_STACK_SIZE    (2048)
#define BUF_SIZE (1024)

static void echo_task(void *arg)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, UART_ECHO_TXD, UART_ECHO_RXD, UART_ECHO_RTS, UART_ECHO_CTS));

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    static int uart_read_count = 0;

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        // Write data back to the UART
        uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) data, len);
        if (len) {
            data[len] = '\0';
            // ESP_LOGI(TAG, "Recv command: %s", (char *)data);
            char command;
            float value = 0.0f;
            float x_value = 0.0f;
            float y_value = 0.0f;

            if (sscanf((const char *)data, "x%f y%f", &x_value, &y_value) == 2) {
                uart_read_count = 0;
                ESP_LOGI(TAG, "Parsed x: %.2f, y: %.2f\n", x_value, y_value);
                spark_bot_motion_control(x_value, y_value);
            } else if (sscanf((const char *)data, "%c%f", &command, &value) == 2) {
                ESP_LOGI(TAG, "Command: %c, Value: %.2f", command, value);
                if (command == 'w') {
                    rgb_ws2812_mode_set((light_mode_t)value);
                    set_power_current_light_mode((light_mode_t)value);
                } else if (command == 'c') {
                    set_motor_speed_coefficients(value);
                } else if (command == 'd') {
                    spark_bot_dance();
                }
            } else {
                ESP_LOGW(TAG, "Unknown command: %s", (char *)data);
            }
        }
        uart_read_count++;
        if (uart_read_count > 25) {
            uart_read_count = 0;
            spark_bot_motion_control(0, 0);
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Let's go!");
    motor_ledc_init();
    rgb_ws2812_init();
    battery_voltage_monitor_start();
    xTaskCreate(echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 10, NULL);
}
