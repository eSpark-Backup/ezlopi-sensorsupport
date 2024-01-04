set(srcs "led_server.c" )

idf_component_register(	SRCS ${srcs}
						INCLUDE_DIRS "." "./compressed_webpage_files"
                        REQUIRES
                            nvs_flash
                            esp_http_client
                            esp_https_server
                            esp_netif
                            esp_wifi

                            uart_reader
                            spiffs

                            EMBED_TXTFILES "certs/cacert.pem"
                            "certs/prvtkey.pem"
						)