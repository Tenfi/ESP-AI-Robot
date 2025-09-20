idf_component_register(SRC_DIRS "src"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES "esp_event" "json" "esp_http_client" "esp-tls"
                       )
                       
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
