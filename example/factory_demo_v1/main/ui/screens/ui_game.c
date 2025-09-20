#include "../ui.h"
#include "lv_100ask_2048.h"
void ui_game_screen_init(void)
{
    ui_game = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_game, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    
    ui_gamepanel = lv_obj_create(ui_game);
    lv_obj_set_width(ui_gamepanel, 240);
    lv_obj_set_height(ui_gamepanel, 240);
    lv_obj_set_align(ui_gamepanel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_gamepanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_gamepanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_gamepanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_gamepanel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_gamepanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_gamepanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_gamepanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_gamepanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_gamepanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    ui_2048 = lv_100ask_2048_create(ui_gamepanel);
    lv_obj_set_size(ui_2048, 240, 200);
    lv_obj_set_align(ui_2048, LV_ALIGN_BOTTOM_MID);

    ui_score = lv_label_create(ui_gamepanel);
    lv_label_set_recolor(ui_score, true);
    lv_label_set_text_fmt(ui_score, "#FFFFFF SCORE:# #FF0000 %d#", lv_100ask_2048_get_score(ui_2048));
    lv_obj_align_to(ui_score, ui_2048, LV_ALIGN_TOP_MID, 0, -20);

    lv_obj_add_event_cb(ui_game, ui_event_game, LV_EVENT_ALL, ui_2048);
    lv_obj_add_event_cb(ui_2048, ui_event_game_score, LV_EVENT_ALL, ui_score);

}