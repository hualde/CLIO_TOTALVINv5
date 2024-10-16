idf_component_register(SRCS "main.c"
                            "can_communication.c"
                            "vin_handling.c"
                            "immo_handling.c"
                            "dtc_handling.c"
                            "utils.c"
                            "wifi_server.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format= -Wno-format)