
file(GLOB_RECURSE config_src "*.c")
# set(config_src)

idf_component_register(SRCS "device_0069_DeepSleep.c" "${config_src}"
    INCLUDE_DIRS "."
    REQUIRES 
        ezlopi-util-trace
        ezlopi-core-actions  
        ezlopi-core-timer 
        ezlopi-core-devices
        ezlopi-cloud-items 
        ezlopi-cloud-constants
        ezlopi-core-devices
        ezlopi-core-valueformatter

        )