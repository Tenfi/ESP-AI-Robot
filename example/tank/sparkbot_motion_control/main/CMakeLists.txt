idf_component_register(
                    SRC_DIRS 
                        "."
                        "./ui"
                        "./ui/screens"
                        "app"
                        "./lottie_player"
                        "./server"
                    INCLUDE_DIRS 
                        "."
                        "./ui"
                        "app"
                        "./lottie_player"
                        "./server"
                    )

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)

spiffs_create_partition_assets(
    animation
    "../lottie_assets"
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".bmp,.json"
)

if(EXISTS ../web_remote_control/dist/index.html)
    spiffs_create_partition_image(
        webserver
        "../web_remote_control/dist"
        FLASH_IN_PROJECT
    )
else()
    message(FATAL_ERROR "../web_remote_control/dist doesn't exit. Please run 'yarn build' in ../web_remote_control/")
endif()