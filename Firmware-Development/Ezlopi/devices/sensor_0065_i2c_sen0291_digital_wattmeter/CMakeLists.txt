idf_component_register(SRCS "sensor_0065_i2c_sen0291_digital_wattmeter.c" "driver/ina219.c"
                        INCLUDE_DIRS "driver" "."
                        REQUIRES ezlopi_actions ezlopi_hal ezlopi_devices ezlopi_cloud_constants trace)