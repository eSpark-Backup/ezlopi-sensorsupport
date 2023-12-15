#include "dht21.h"
#ifndef SENSOR_0067_ONEWIRE_DHT21_H
#define SENSOR_0067_ONEWIRE_DHT21_H

#include "ezlopi_actions.h"
#include "ezlopi_devices.h"

typedef struct ezlopi_dht21_data
{
    float temperature;
    float humidity;
    struct dht21 sensor_data;

} s_ezlopi_dht21_data_t;

int sensor_0067_oneWire_DHT21(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg);

#endif // SENSOR_0016_ONEWIRE_DHT22_H
