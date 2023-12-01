

#ifndef SENSOR_0067_ONEWIRE_DHT22_H
#define SENSOR_0067_ONEWIRE_DHT22_H

#include "ezlopi_actions.h"
#include "ezlopi_devices.h"

typedef struct ezlopi_dht22_data2
{
    float temperature;
    float humidity;
} s_ezlopi_dht22_data_t2;

int sensor_0067_oneWire_DHT22(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg);

#endif // SENSOR_0016_ONEWIRE_DHT22_H
