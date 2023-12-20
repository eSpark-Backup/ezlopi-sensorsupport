

idf_component_register(SRCS "item_prepare.c" "get_cjson_value.c" "device_0068_other_addressable_RGB.c"
                            "set_cjson_value.c"
                        INCLUDE_DIRS "."
                        REQUIRES trace 
                        led_strip
                        ezlopi_actions 
                        ezlopi_hal 
                        ezlopi_timer 
                        ezlopi_devices 
                        ezlopi_cloud_constants 
                        ezlopi_valueformatter
                        gpio_isr_service

                        led
                        config
                        )

