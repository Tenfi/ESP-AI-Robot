/**
 * @file lv_lottie.h
 *
 */

#ifndef LV_LOTTIE_H
#define LV_LOTTIE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create a lottie animation
 * @param parent    pointer to the parent widget
 * @return          pointer to the created Lottie animation widget
 */
lv_obj_t   *lv_lottie_create(lv_obj_t *parent);

/**
 * Resize a Lottie animation widget and update its dimensions.
 * @param obj   pointer to a Lottie widget
 * @param w     new width for the Lottie widget
 * @param h     new height for the Lottie widget
 */
void lv_lottie_resize(lv_obj_t *obj, int32_t w, int32_t h);

/**
 * Set a buffer for the animation. It also defines the size of the animation
 * @param obj       pointer to a lottie widget
 * @param w         width of the animation and buffer
 * @param h         height of the animation and buffer
 * @param buf       a static buffer with `width x height x 4` byte size
 */
void lv_lottie_set_buffer(lv_obj_t *obj, int32_t w, int32_t h, void *buf);

/**
 * Set a draw buffer for the animation. It also defines the size of the animation
 * @param obj       pointer to a lottie widget
 * @param draw_buf  an initialized draw buffer with ARGB8888 color format
 */
void lv_lottie_set_draw_buf(lv_obj_t *obj, lv_img_dsc_t *draw_buf);

/**
 * Set the source for the animation as an array
 * @param obj       pointer to a lottie widget
 * @param src       the lottie animation converted to an nul terminated array
 * @param src_size  size of the source array in bytes
 */
void lv_lottie_set_src_data(lv_obj_t *obj, const void *src, size_t src_size);

/**
 * Set the source for the animation as a path.
 * Lottie doesn't use LVGL's File System API.
 * @param obj       pointer to a lottie widget
 * @param src       path to a json file, e.g. "path/to/file.json"
 */
void lv_lottie_set_src_file(lv_obj_t *obj, const char *src);

/**
 * Get the LVGL animation which controls the lottie animation
 * @param obj       pointer to a lottie widget
 * @return          the LVGL animation
 */
lv_anim_t *lv_lottie_get_anim(lv_obj_t *obj);

/**
 * Set the playback frame range for a Lottie animation with additional control parameters.
 *
 * This function allows setting the frame range, splitting playback into segments,
 * and controlling loop behavior.
 *
 * @param obj       pointer to a Lottie widget
 * @param begin     the first frame to play
 * @param end       the last frame to play
 * @param loop      whether the animation should loop (true to loop, false to stop after the range)
 */
void lv_lottie_set_segment(lv_obj_t *obj, uint32_t begin, uint32_t end, bool loop);

/**
 * Get the start frame of a Lottie animation.
 * This function retrieves the starting frame of the animation range for the specified Lottie widget.
 *
 * @param obj   pointer to a Lottie widget
 * @return      the starting frame of the animation range as a float
 */
uint32_t lv_lottie_get_start_f(lv_obj_t * obj);

/**********************
 * GLOBAL VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_LOTTIE_H*/
