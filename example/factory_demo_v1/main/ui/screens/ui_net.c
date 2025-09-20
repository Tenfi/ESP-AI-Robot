#include "../ui.h"
void ui_net_screen_init(void)
{
    ui_net = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_net, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    
    ui_netpanel = lv_obj_create(ui_net);
    lv_obj_set_width(ui_netpanel, 240);
    lv_obj_set_height(ui_netpanel, 240);
    lv_obj_set_align(ui_netpanel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_netpanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_netpanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_netpanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_netpanel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_netpanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_netpanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_netpanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_netpanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_netpanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);

    lv_obj_t * qr = lv_qrcode_create(ui_netpanel, 150, fg_color, bg_color);
    /* Set data*/
    lv_qrcode_update(qr, url, strlen(url));
    lv_obj_center(qr);

    /*Add a border with bg_color*/
    lv_obj_set_style_border_color(qr, bg_color, 0);
    lv_obj_set_style_border_width(qr, 5, 0);
    lv_obj_add_event_cb(ui_net, ui_event_net, LV_EVENT_ALL, NULL);
}