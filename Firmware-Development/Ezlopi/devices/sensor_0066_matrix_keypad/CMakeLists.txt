idf_component_register(SRCS "sensor_0066_matrix_keypad.c" "driver/keypad.c"
                        INCLUDE_DIRS "driver" "."
                        REQUIRES ezlopi_actions ezlopi_hal ezlopi_devices ezlopi_cloud_constants trace)