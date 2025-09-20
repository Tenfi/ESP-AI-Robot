#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POWER_ADC_CHANNEL               ADC_CHANNEL_3    // Power ADC channel
#define POWER_VOLTAGE_RATIO             2.0f             // Power voltage ratio

/**
 * @brief   Power Init
 * @note    This function is used to initialize the power module.
 *          It initializes the ADC2 channel 6 and the GPIO input pin.
 *          It also performs the ADC calibration.
 */
void power_adc_init(void);

/**
 * @brief   Get Power Value
 * @note    This function is used to get the power value.
 *          It gets the power voltage and converts it to the power value.
 * @return  The power value.
 */
int get_power_value(void);


#ifdef __cplusplus
}
#endif