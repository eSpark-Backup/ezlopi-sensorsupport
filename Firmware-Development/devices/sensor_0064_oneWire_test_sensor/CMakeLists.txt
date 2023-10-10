idf_component_register(SRCS "driver/a4988.c" "device_0064_motor_driver.c"
                        INCLUDE_DIRS "driver" "."
                        REQUIRES trace ezlopi_actions ezlopi_hal ezlopi_timer ezlopi_devices ezlopi_cloud_constants gpio_isr_service trace)