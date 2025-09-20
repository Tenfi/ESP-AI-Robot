#ifndef LV_100ASK_2048_H
#define LV_100ASK_2048_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

#define MATRIX_SIZE     4

typedef struct {
    lv_obj_t obj;
    lv_obj_t * btnm;
    uint16_t score;
    uint16_t map_count;
    uint16_t matrix[MATRIX_SIZE][MATRIX_SIZE];
    char   * btnm_map[MATRIX_SIZE * MATRIX_SIZE + MATRIX_SIZE];
    bool     game_over;
} lv_100ask_2048_t;

lv_obj_t * lv_100ask_2048_create(lv_obj_t * parent);

void lv_100ask_2048_set_new_game(lv_obj_t * obj);

uint16_t lv_100ask_2048_get_best_tile(lv_obj_t * obj);

uint16_t lv_100ask_2048_get_score(lv_obj_t * obj);

bool lv_100ask_2048_get_status(lv_obj_t * obj);

#endif  /*LV_USE_100ASK_2048*/

#ifdef __cplusplus
} /*extern "C"*/
#endif
