
idf_component_register(
    SRC_DIRS 
        "." 
        "./ui"
        "./ui/screens"
        "./ui/fonts"
        "./ui/images"
        "./app"
        "./tinyGL_cube"
        "./lottie_player"
        "./game_2048"
    INCLUDE_DIRS 
        "." 
        "./ui" 
        "./app"
        "./tinyGL_cube"
        "./lottie_player"
        "./game_2048"
    )
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format" "-Wno-unused-variable")

idf_component_get_property(lib espressif2022__tinygl COMPONENT_LIB)
target_compile_options(${lib} PRIVATE -Wno-implicit-fallthrough -Wno-unused-variable -Wno-unused-function)

# Determine whether esp-sr is fetched from component registry or from local path
idf_build_get_property(build_components BUILD_COMPONENTS)
if(esp-sr IN_LIST build_components)
    set(esp-sr_name esp-sr) # Local component
else()
    set(esp-sr_name espressif__esp-sr) # Managed component
endif()

spiffs_create_partition_assets(
    animation
    "../lottie_assets"
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".bmp,.json"
)

spiffs_create_partition_assets(
    storage
    "../audio"
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".wav"
)

spiffs_create_partition_assets(
    weather
    "../weather"
    FLASH_IN_PROJECT
    MMAP_FILE_SUPPORT_FORMAT ".jpg,.png"
    MMAP_SUPPORT_QOI
)