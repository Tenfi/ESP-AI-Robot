#include "../ui.h"
#include "lv_lottie.h"

void ui_fish_screen_init(void)
{
    ui_fish = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_fish, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_Panel_fish = lv_obj_create(ui_fish);
    lv_obj_set_width(ui_Panel_fish, 240);
    lv_obj_set_height(ui_Panel_fish, 240);
    lv_obj_set_align(ui_Panel_fish, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Panel_fish, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_Panel_fish, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Panel_fish, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Panel_fish, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Panel_fish, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_Panel_fish, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_Panel_fish, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_Panel_fish, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_Panel_fish, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_fish_canvas = lv_lottie_create(ui_Panel_fish);
    lv_obj_align(ui_fish_canvas, LV_ALIGN_CENTER, -240, 0);

    static uint8_t *fb = NULL;;
    if (fb == NULL) {
        fb = malloc(LOTTIE_SIZE_HOR_MIN * LOTTIE_SIZE_VER_MIN * 4);
        assert(fb);
        lv_lottie_set_buffer(ui_fish_canvas, LOTTIE_SIZE_HOR_MIN, LOTTIE_SIZE_VER_MIN, fb);
    }

    lv_obj_add_event_cb(ui_fish, ui_event_fish, LV_EVENT_ALL, NULL);
}
