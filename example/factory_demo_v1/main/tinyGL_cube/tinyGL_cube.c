/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"

#include "zbuffer.h"
#include <GL/gl.h>
#include "glu.h"

#include "tinyGL_cube.h"

static const char *TAG = "tintGL_cube";

#define MY_CLASS                &lv_cube_class
#define LV_RGB24                24

typedef struct {
    lv_img_t img_ext;
    lv_img_dsc_t imgdsc;

    ZBuffer *allocated_buf;
    GLuint cube_tex[6];
} lv_cube_t;

static void lv_cube_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_cube_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_cube_event(const lv_obj_class_t *class_p, lv_event_t *e);

const lv_obj_class_t lv_cube_class = {
    .constructor_cb = lv_cube_constructor,
    .destructor_cb  = lv_cube_destructor,
    .event_cb       = lv_cube_event,
    .instance_size  = sizeof(lv_cube_t),
    .base_class     = &lv_img_class
};

typedef struct {
    int32_t width;
    int32_t height;
    size_t tex_count;
    cube_tex_t *texture;
} lv_cube_create_info_t;

/*Only used in lv_obj_class_create_obj, no affect multiple instances*/
static lv_cube_create_info_t create_info;

typedef struct {
    float v1;
    float v2;
    float v3;
} V3;

typedef struct {
    float t1;
    float t2;
} T2;

static V3 ptrv[8] = {
    {-1.0f, 1.0f, 1.0f},
    {-1.0f, -1.0f, 1.0f},
    {1.0f, -1.0f, 1.0f},
    {1.0f, 1.0f, 1.0f},
    {-1.0f, 1.0f, -1.0f},
    {-1.0f, -1.0f, -1.0f},
    {1.0f, -1.0f, -1.0f},
    {1.0f, 1.0f, -1.0f}
};

static T2 ptrt[4] = {
    {0.0, 0.0},
    {1.0, 0.0},
    {1.0, 1.0},
    {0.0, 1.0}
};

/**********************
 *   STATIC FUNCTIONS
 **********************/
#if LV_COLOR_DEPTH == 16
static void convert_to_rgb565(uint8_t *in, int width, int height)
{
    uint16_t *out = (uint16_t *)in;
    int numPixels = width * height;

    for (int i = 0; i < numPixels; i++) {
        uint8_t r = in[i * 3 + 2];
        uint8_t g = in[i * 3 + 1];
        uint8_t b = in[i * 3 + 0];

        uint16_t r565 = (r >> 3) & 0x1F;
        uint16_t g565 = (g >> 2) & 0x3F;
        uint16_t b565 = (b >> 3) & 0x1F;

        uint16_t rgb565Value = (r565 << 11) | (g565 << 5) | b565;
#if LV_COLOR_16_SWAP == 0
        out[i] = rgb565Value;
#else
        out[i] = (((rgb565Value) >> 8) | (((rgb565Value) & 0xFF) << 8));
#endif
    }
}
#endif

static void cube_draw_quads(V3 *ptr, int iv1, int iv2, int iv3, int iv4, T2 *ptrt, int it1, int it2, int it3, int it4, GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);

    glBegin(GL_QUADS);
    glTexCoord2fv((float *)&ptrt[it1]);
    glVertex3fv((float *)&ptr[iv1]);

    glTexCoord2fv((float *)&ptrt[it2]);
    glVertex3fv((float *)&ptr[iv2]);

    glTexCoord2fv((float *)&ptrt[it3]);
    glVertex3fv((float *)&ptr[iv3]);

    glTexCoord2fv((float *)&ptrt[it4]);
    glVertex3fv((float *)&ptr[iv4]);

    glEnd();
}

static void cube_reshape()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0, 1.0, 1.0, 300.0);
    glMatrixMode(GL_MODELVIEW);
}

static GLuint cube_load_texture(cube_tex_t *texture)
{
    void *image = texture->data;
    int width = texture->tex_width;
    int height = texture->tex_height;

    glEnable(GL_DEPTH_TEST);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    GLuint cube_tex;
    glGenTextures(1, &cube_tex);
    glBindTexture(GL_TEXTURE_2D, cube_tex);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0,  GL_RGB, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, cube_tex);
    glEnable(GL_TEXTURE_2D);

    return cube_tex;
}

esp_err_t cube_update(lv_obj_t *obj, float x, float y, float z)
{
    lv_cube_t *tinyGL = (lv_cube_t *)obj;

    glLoadIdentity();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glTranslatef(0.0, 0.0, -6.0);

    glRotatef(x, 1.0f, 0.0f, 0.0f);
    glRotatef(y, 0.0f, 1.0f, 0.0f);
    glRotatef(z, 0.0f, 0.0f, 1.0f);

    cube_draw_quads((V3 *)&ptrv, 0, 1, 2, 3, (T2 *)&ptrt, 3, 0, 1, 2, tinyGL->cube_tex[0]); //front
    cube_draw_quads((V3 *)&ptrv, 0, 3, 7, 4, (T2 *)&ptrt, 1, 2, 3, 0, tinyGL->cube_tex[1]); //left
    cube_draw_quads((V3 *)&ptrv, 4, 7, 6, 5, (T2 *)&ptrt, 2, 3, 0, 1, tinyGL->cube_tex[2]); //back
    cube_draw_quads((V3 *)&ptrv, 5, 6, 2, 1, (T2 *)&ptrt, 3, 0, 1, 2, tinyGL->cube_tex[3]); //right
    cube_draw_quads((V3 *)&ptrv, 7, 3, 2, 6, (T2 *)&ptrt, 3, 0, 1, 2, tinyGL->cube_tex[4]); //top
    cube_draw_quads((V3 *)&ptrv, 5, 1, 0, 4, (T2 *)&ptrt, 3, 0, 1, 2, tinyGL->cube_tex[5]); //bottom

    return ESP_OK;
}

static void lv_cube_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_cube_t *tinyGL = (lv_cube_t *)obj;

    tinyGL->allocated_buf = ZB_open(create_info.width, create_info.height, ZB_MODE_RGB24, 0, NULL, NULL, NULL);

    GLfloat h = (GLfloat)create_info.width / (GLfloat)create_info.height;

    glInit(tinyGL->allocated_buf);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, create_info.width, create_info.height);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -45.0);

    for (int i = 0; i < create_info.tex_count; i++) {
        tinyGL->cube_tex[i] = cube_load_texture(&create_info.texture[i]);
    }
    cube_reshape();

    size_t allocated_buf_size = (create_info.width * create_info.height * LV_RGB24 / 8);

    tinyGL->imgdsc.header.cf = LV_IMG_CF_TRUE_COLOR;
    tinyGL->imgdsc.header.h = create_info.height;
    tinyGL->imgdsc.header.w = create_info.width;
    tinyGL->imgdsc.data = (void *)tinyGL->allocated_buf->pbuf;
    tinyGL->imgdsc.data_size = allocated_buf_size;

    lv_img_set_src(obj, &tinyGL->imgdsc);
    cube_update(obj, 0, 0, 0);

#if LV_COLOR_DEPTH == 16
    convert_to_rgb565(tinyGL->allocated_buf->pbuf, tinyGL->imgdsc.header.w, tinyGL->imgdsc.header.h);
#endif

    lv_obj_update_layout(obj);

    LV_TRACE_OBJ_CREATE("finished");
    ESP_LOGI(TAG, "cube_constructor");
}

static void lv_cube_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);

    lv_cube_t *tinyGL = (lv_cube_t *)obj;
    lv_img_cache_invalidate_src(&tinyGL->imgdsc);

    glDisable(GL_TEXTURE_2D);

    glDeleteTextures(sizeof(tinyGL->cube_tex) / sizeof(GLuint), tinyGL->cube_tex);  // Delete the texture object
    ZB_close(tinyGL->allocated_buf);

    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (create_info.tex_count) {
        free(create_info.texture);
    }
    glClose();
    ESP_LOGI(TAG, "cube_destructor");
}

static void lv_cube_event(const lv_obj_class_t *class_p, lv_event_t *e)
{
    LV_UNUSED(class_p);

    lv_res_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if (res != LV_RES_OK) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_current_target(e);
    bmi270_axis_t *rot = lv_event_get_param(e);

    lv_cube_t *tinyGL = (lv_cube_t *)obj;

    if (code == LV_EVENT_VALUE_CHANGED) {

        // ESP_LOGI(TAG, "changed, %.2f, %.2f, %.2f", rot->pitch, rot->roll, rot->yaw);
        cube_update(obj, rot->pitch, rot->roll, rot->yaw);

#if LV_COLOR_DEPTH == 16
        convert_to_rgb565(tinyGL->allocated_buf->pbuf, tinyGL->imgdsc.header.w, tinyGL->imgdsc.header.h);
#endif
        lv_obj_invalidate(obj);
    }
}

lv_obj_t *lv_tinyGL_create_cube(lv_obj_t *parent, int32_t width, int32_t height, cube_tex_t *tex_list, size_t tex_count)
{
    if (tex_list == NULL || tex_count == 0) {
        LV_LOG_WARN("No textures provided");
        return NULL;
    }

    create_info.tex_count = tex_count;
    create_info.texture = malloc(sizeof(cube_tex_t) * tex_count);
    memcpy(create_info.texture, tex_list, sizeof(cube_tex_t) * tex_count);

    create_info.width = width;
    create_info.height = height;

    LV_LOG_INFO("begin");

    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);

    return obj;
}
