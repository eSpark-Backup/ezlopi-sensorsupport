#ifndef __TFMINI_H__
#define __TFMINI_H__

#include "ezlopi_action.h"
#include "ezlopi_devices.h"

#define TFMINI_BUFFER_SIZE 8
uint8_t buffer[TFMINI_BUFFER_SIZE];

typedef struct TFMINIdata
{
    uint8_t tfmini_distance,
        tfmini_chip_temprature,
        tfmini_strength;
} TFMINI_Data;

typedef struct 
{
    TFMINI_Data* tfmini_data;
} TFMINI_sensor_structure_t;

void tfmini_uart_setup(TFMINI_sensor_structure_t* tfmini_sensor_structure, uint32_t tx, uint32_t rx, uint32_t buadrate);
esp_err_t tfmini_read_upcall(uint8_t* buffer,TFMINI_Data* data);

void tfmini_print_data(TFMINI_Data* data);


#endif //__TFMINI_H__