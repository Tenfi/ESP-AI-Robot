#include "../ui.h"
void ui_camera_screen_init(void)
{
    ui_camera = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_camera, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    
    ui_panel_camera = lv_obj_create(ui_camera);
    lv_obj_set_width(ui_panel_camera, 240);
    lv_obj_set_height(ui_panel_camera, 240);
    lv_obj_set_align(ui_panel_camera, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_panel_camera, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_panel_camera, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_panel_camera, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_panel_camera, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_panel_camera, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_panel_camera, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_panel_camera, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_panel_camera, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_panel_camera, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_camera_canvas = lv_canvas_create(ui_panel_camera);
    lv_obj_align(ui_camera_canvas, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(ui_camera_canvas, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_add_event_cb(ui_camera, ui_event_camera, LV_EVENT_ALL, NULL);
}