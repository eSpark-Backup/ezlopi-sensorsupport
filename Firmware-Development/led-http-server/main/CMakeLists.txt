idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."

                    REQUIRES 
                        led_server
                        led
                        config
                        uart_reader
                    )
# spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)