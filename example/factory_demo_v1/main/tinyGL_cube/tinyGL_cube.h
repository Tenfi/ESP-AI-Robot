/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __CUBETEXT_H__
#define __CUBETEXT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
typedef struct {
    void *data;
    uint16_t tex_width;
    uint16_t tex_height;
} cube_tex_t;

typedef struct {
    float pitch;
    float yaw;
    float roll;
} bmi270_axis_t;

lv_obj_t *lv_tinyGL_create_cube(lv_obj_t *parent, int32_t width, int32_t height, cube_tex_t *tex_list, size_t tex_count);

#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif
