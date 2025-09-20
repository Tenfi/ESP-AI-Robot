#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void set_power_current_light_mode(light_mode_t mode);
void battery_voltage_monitor_start(void);

#ifdef __cplusplus
}
#endif
