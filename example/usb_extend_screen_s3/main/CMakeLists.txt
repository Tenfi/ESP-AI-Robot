set(srcs "app_hid.c" "app_touch.c" "app_uac.c" "app_usb.c" "app_vendor.c"
         "usb_extend_screen.c" "usb_frame.c" "uac/usb_device_uac.c" "usb_device/usb_descriptors.c")

if(CONFIG_IDF_TARGET_ESP32S3)
    list(APPEND srcs "app_lcd_s3.c")
endif()

idf_component_register(SRCS ${srcs}
                    #    SRC_DIRS "usb_device" "uac"
                        INCLUDE_DIRS "include" "uac" "usb_device")

idf_component_get_property(tusb_lib leeebo__tinyusb_src COMPONENT_LIB)

cmake_policy(SET CMP0079 NEW)
target_link_libraries(${tusb_lib} PRIVATE ${COMPONENT_LIB})
