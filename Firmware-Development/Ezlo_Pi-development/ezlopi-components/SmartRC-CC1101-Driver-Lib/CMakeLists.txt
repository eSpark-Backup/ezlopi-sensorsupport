idf_component_register(SRCS "ELECHOUSE_CC1101_SRC_DRV.c"
                INCLUDE_DIRS "."
                REQUIRES 
                    driver 
                    rc-switch 
                    ezlopi-hal-spi
                    ezlopi-hal-uart
                    # ezlopi_uart 
                    json
                    )