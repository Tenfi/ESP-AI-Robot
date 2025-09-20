idf_component_register(SRC_DIRS "src"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES "esp_event"
                       )
                       
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
