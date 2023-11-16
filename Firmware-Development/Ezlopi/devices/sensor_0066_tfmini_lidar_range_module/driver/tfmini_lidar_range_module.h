#ifndef __TFMINI_H__
#define __TFMINI_H__

#include "ezlopi_actions.h"
#include "ezlopi_devices.h"

typedef struct TFMINIdata
{
    uint8_t tfmini_distance,
        tfmini_chip_temperature,
        tfmini_strength;
    uint32_t distance_item_id;    
} TFMINI_Data;

s_ezlopi_uart_object_handle_t tfmini_init(uint32_t baudrate, uint32_t tx, uint32_t rx, TFMINI_Data* tfmini_sensor_data_structure);
void tfmini_update_data(uint8_t* buffer, TFMINI_Data* tfmini_sensor, s_ezlopi_uart_object_handle_t uart_object_handle);
static void tfmini_upcall(uint8_t* buffer, s_ezlopi_uart_object_handle_t s_ezlopi_uart_object);
void tfmini_parse_and_assign(uint8_t* buffer,TFMINI_Data* data);

#endif //__TFMINI_H__