idf_component_register(SRCS "device_0064_motor_driver_a4988.c" "driver/a4988.c"
                        INCLUDE_DIRS "driver" "."
                        REQUIRES trace ezlopi_actions ezlopi_hal ezlopi_timer ezlopi_devices ezlopi_cloud_constants gpio_isr_service trace)