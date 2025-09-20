idf_component_register(SRCS "power_management.c"
                    INCLUDE_DIRS "include"
                    REQUIRES light_control
                    PRIV_REQUIRES esp_adc driver)
