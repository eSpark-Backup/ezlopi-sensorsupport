

idf_component_register(SRCS "device_0068_other_addressable_RGB.c"
                        INCLUDE_DIRS "."
                        REQUIRES trace 
                        led_strip
                        ezlopi_actions 
                        ezlopi_hal 
                        ezlopi_timer 
                        ezlopi_devices 
                        ezlopi_cloud_constants 
                        gpio_isr_service
                        )

