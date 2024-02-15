
file(GLOB_RECURSE config_src "*.c")
# set(config_src)

idf_component_register(SRCS "device_0070_DeepSleep.c" "${config_src}"
    INCLUDE_DIRS "."
    REQUIRES 
        ezlopi-core-actions  
        ezlopi-core-devices
        ezlopi-cloud-items 
        ezlopi-cloud-constants
        )