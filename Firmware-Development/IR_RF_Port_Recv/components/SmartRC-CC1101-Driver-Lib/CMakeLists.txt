idf_component_register(SRCS "ELECHOUSE_CC1101_SRC_DRV.c"
                    REQUIRES driver rc-switch ezlopi_spi ezlopi_uart json
                    INCLUDE_DIRS "."
)