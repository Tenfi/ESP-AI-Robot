#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "light_control.h"
#include "power_management.h"

static const char *TAG = "power_management";

#define VBAT_ADC_CHAN           ADC_CHANNEL_3
#define VBAT_ADC_ATTEN          ADC_ATTEN_DB_12
#define VBAT_MONITOR_GPIO       (GPIO_NUM_2)
#define VBAT_CHARGING_LEVEL     0
#define BATTERY_VOLTAGE_PERCENT 20

#define ADC_SMOOTH_WINDOW_SIZE  10

static int adc_raw[2][10];
static int voltage[2][10];

static light_mode_t power_current_light_mode = LIGHT_MODE_SLEEP;

static int battery_voltage_smoothing(int new_sample)
{
    static int battery_voltage_samples[ADC_SMOOTH_WINDOW_SIZE] = {0};
    static uint8_t battery_voltage_index = 0;
    static int battery_voltage_sum = 0;

    battery_voltage_sum -= battery_voltage_samples[battery_voltage_index];
    battery_voltage_samples[battery_voltage_index] = new_sample;
    battery_voltage_sum += new_sample;
    battery_voltage_index = (battery_voltage_index + 1) % ADC_SMOOTH_WINDOW_SIZE;

    return battery_voltage_sum / ADC_SMOOTH_WINDOW_SIZE;
}

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void vbat_monitor_task(void *arg)
{
    gpio_config_t vbat_monitor_io_config = {
        .pin_bit_mask = 1ULL << VBAT_MONITOR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&vbat_monitor_io_config);
    static int last_vbat_charging_state = !VBAT_CHARGING_LEVEL;

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = VBAT_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, VBAT_ADC_CHAN, &config));

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, VBAT_ADC_CHAN, VBAT_ADC_ATTEN, &adc1_cali_chan0_handle);

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, VBAT_ADC_CHAN, &adc_raw[0][0]));
        if (do_calibration1_chan0) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0][0], &voltage[0][0]));
        }

        int battery_voltage = voltage[0][0] * 2;
        // printf("battery voltage: %d\n", battery_voltage);
        battery_voltage = battery_voltage_smoothing(battery_voltage);
        float voltage_percent = ((battery_voltage - 3300) / (4200.0 - 3300)) * 100.0;
        int battery_voltage_percent = (int)voltage_percent;
        static int battery_voltage_percent_last = 75;
        if (battery_voltage_percent_last != battery_voltage_percent) {
            // ESP_LOGW(TAG, "battery voltage percent: %d", battery_voltage_percent);
            battery_voltage_percent_last = battery_voltage_percent;
        }

        int vbat_charging_state = gpio_get_level(VBAT_MONITOR_GPIO);

        if (vbat_charging_state != last_vbat_charging_state) {
            last_vbat_charging_state = vbat_charging_state;
            if (vbat_charging_state == VBAT_CHARGING_LEVEL) {
                // ESP_LOGI(TAG, "The battery starts charging.");
            } else {
                // ESP_LOGI(TAG, "The battery is not charging.");
            }
        }

        static light_mode_t charging_light_mode = LIGHT_MODE_WHITE_BREATH_SLOW;
        static light_mode_t charging_light_mode_last = LIGHT_MODE_WHITE_BREATH_SLOW;

        if (vbat_charging_state == VBAT_CHARGING_LEVEL) {
            charging_light_mode = LIGHT_MODE_CHARGING_BREATH;
        } else {
            if (battery_voltage_percent <= BATTERY_VOLTAGE_PERCENT && battery_voltage_percent >= 0) {
                charging_light_mode = LIGHT_MODE_POWER_LOW;
            } else {
                charging_light_mode = power_current_light_mode;
            }
        }

        if (charging_light_mode != charging_light_mode_last) {
            rgb_ws2812_mode_set(charging_light_mode);
            charging_light_mode_last = charging_light_mode;
        }

        // ESP_LOGI(TAG, "battery charging state: %d", vbat_charging_state);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void set_power_current_light_mode(light_mode_t mode)
{
    power_current_light_mode = mode;
}

void battery_voltage_monitor_start(void)
{
    xTaskCreate(vbat_monitor_task, "vbat_monitor_task", 1024, NULL, 5, NULL);
}
