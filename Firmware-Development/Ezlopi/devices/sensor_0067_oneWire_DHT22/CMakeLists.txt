idf_component_register(SRCS "sensor_0067_oneWire_DHT22.c" "dht21.c"
                        INCLUDE_DIRS "."
                        REQUIRES trace library ezlopi_actions ezlopi_hal ezlopi_timer ezlopi_devices ezlopi_cloud_constants gpio_isr_service)