set(srcs "led_server.c" )

idf_component_register(	SRCS ${srcs}
						INCLUDE_DIRS "."
                        REQUIRES
                            nvs_flash
                            esp_http_client
                            esp_https_server
                            esp_netif
                            esp_wifi

                            uart_reader
                            spiffs
						)