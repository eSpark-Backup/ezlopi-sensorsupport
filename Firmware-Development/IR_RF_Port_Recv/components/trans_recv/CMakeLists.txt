idf_component_register(SRCS "trans_recv.c" 
    REQUIRES 
            driver 
            trace 
            SmartRC-CC1101-Driver-Lib
    INCLUDE_DIRS "."
)