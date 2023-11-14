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

void tfmini_parse_and_assign(uint8_t* buffer,TFMINI_Data* data);

#endif //__TFMINI_H__