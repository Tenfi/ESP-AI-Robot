#include "../ui.h"
#include "app_wifi.h"

void ui_title_screen_init(void)
{
    ui_title = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_title, LV_OBJ_FLAG_SCROLLABLE);

    title_panel = lv_obj_create(ui_title);
    lv_obj_set_width(title_panel, 240);
    lv_obj_set_height(title_panel, 20);
    lv_obj_set_align(title_panel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(title_panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(title_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(title_panel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(title_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(title_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(title_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(title_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(title_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(title_panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    title_wifistate = lv_img_create(title_panel);
    lv_img_set_src(title_wifistate, &ui_img_wifi_disconnection_png);
    lv_obj_set_width(title_wifistate, LV_SIZE_CONTENT);   /// 10
    lv_obj_set_height(title_wifistate, LV_SIZE_CONTENT);    /// 10
    lv_obj_set_x(title_wifistate, 55);
    lv_obj_set_y(title_wifistate, 0);
    lv_obj_set_align(title_wifistate, LV_ALIGN_CENTER);
    lv_obj_add_flag(title_wifistate, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(title_wifistate, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(title_wifistate, 400);
    lv_obj_set_style_img_recolor(title_wifistate, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_img_recolor_opa(title_wifistate, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    title_powerstate = lv_slider_create(title_panel);
    lv_slider_set_value(title_powerstate, 50, LV_ANIM_OFF);
    if (lv_slider_get_mode(title_powerstate) == LV_SLIDER_MODE_RANGE) {
        lv_slider_set_left_value(title_powerstate, 0, LV_ANIM_OFF);
    }
    lv_obj_set_width(title_powerstate, 15);
    lv_obj_set_height(title_powerstate, 4);
    lv_obj_set_x(title_powerstate, -8);
    lv_obj_set_y(title_powerstate, 8);
    lv_obj_set_align(title_powerstate, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_style_radius(title_powerstate, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(title_powerstate, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(title_powerstate, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_img_opa(title_powerstate, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(title_powerstate, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(title_powerstate, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(title_powerstate, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(title_powerstate, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(title_powerstate, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_radius(title_powerstate, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(title_powerstate, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(title_powerstate, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(title_powerstate, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(title_powerstate, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    title_batterybor = lv_img_create(title_panel);
    lv_img_set_src(title_batterybor, &ui_img_battery_png);
    lv_obj_set_width(title_batterybor, LV_SIZE_CONTENT);   /// 20
    lv_obj_set_height(title_batterybor, LV_SIZE_CONTENT);    /// 20
    lv_obj_set_x(title_batterybor, 106);
    lv_obj_set_y(title_batterybor, 0);
    lv_obj_set_align(title_batterybor, LV_ALIGN_CENTER);
    lv_obj_add_flag(title_batterybor, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
    lv_obj_clear_flag(title_batterybor, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_img_set_zoom(title_batterybor, 400);
    lv_obj_set_style_img_recolor(title_batterybor, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_img_recolor_opa(title_batterybor, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    title_timestate = lv_label_create(title_panel);
    lv_obj_set_width(title_timestate, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(title_timestate, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(title_timestate, -95);
    lv_obj_set_y(title_timestate, 0);
    lv_obj_set_align(title_timestate, LV_ALIGN_CENTER);
    lv_label_set_text(title_timestate, "23:59");
    lv_obj_set_style_text_color(title_timestate, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(title_timestate, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title_timestate, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    title_batterytxt = lv_label_create(title_panel);
    lv_obj_set_width(title_batterytxt, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(title_batterytxt, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(title_batterytxt, 80);
    lv_obj_set_y(title_batterytxt, 0);
    lv_obj_set_align(title_batterytxt, LV_ALIGN_CENTER);
    lv_label_set_text(title_batterytxt, "--%%");
    lv_obj_set_style_text_color(title_batterytxt, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(title_batterytxt, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title_batterytxt, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);
}
