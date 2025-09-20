#ifndef _APP_DATAFUSION_H_
#define _APP_DATAFUSION_H_
#include "app_imu.h"
#include "tinyGL_cube.h"

#ifdef __cplusplus
extern "C" {
#endif

void MahonyAHRSupdateIMU(float gx, float gy, float gz, float ax, float ay,
                         float az, bmi270_axis_t * axis_val);

void calculateAttitude(float gyroX, float gyroY, float gyroZ, float accX, float accY, float accZ, float dt, bmi270_axis_t * axis_val);

#ifdef __cplusplus
}
#endif

#endif