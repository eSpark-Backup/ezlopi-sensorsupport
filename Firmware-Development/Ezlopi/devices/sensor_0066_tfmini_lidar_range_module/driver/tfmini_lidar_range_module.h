#ifndef __TFMINI_H__
#define __TFMINI_H__

#include "ezlopi_actions.h"
#include "ezlopi_devices.h"

typedef struct tfmini_data
{
    uint8_t tfmini_distance,
        tfmini_chip_temperature,
        tfmini_strength;   
} tfmini_data_t;

s_ezlopi_uart_object_handle_t tfmini_init(uint32_t baudrate, uint32_t tx, uint32_t rx, tfmini_data_t* tfmini_sensor_data_structure);

#endif //__TFMINI_H__