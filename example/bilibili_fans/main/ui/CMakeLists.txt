SET(SOURCES screens/ui_Screen1.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_bilibili_png.c
    images/ui_img_bilibiliname_png.c
    images/ui_img_bilibili_name_2_png.c
    fonts/ui_font_PHFont10.c
    fonts/ui_font_PHFont14.c
    fonts/ui_font_PHFont16.c
    fonts/ui_font_PHFont28.c
    fonts/ui_font_PHFont17.c)

add_library(ui ${SOURCES})
