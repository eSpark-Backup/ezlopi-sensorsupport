
#include <math.h>
#include "cJSON.h"
#include "ezlopi_cloud.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_item_name_str.h"
#include "ezlopi_device_value_updated.h"
#include "ezlopi_cloud_category_str.h"
#include "ezlopi_cloud_subcategory_str.h"
#include "ezlopi_cloud_device_types_str.h"
#include "ezlopi_cloud_value_type_str.h"
#include "ezlopi_cloud_scales_str.h"
#include "ezlopi_valueformatter.h"

#include "trace.h"
#include "ezlopi_adc.h"
#include "dht22.h"
#include "dht21.h" //include header files
#include "utils.h" //include header files
float temp_in_float; //for float
float humidity_in_float; //for humidity


#include "sensor_0067_oneWire_DHT22.h"

struct dht21 sensor_data;//@

static int dht22_sensor_prepare_v3(void *arg);
static int dht22_sensor_init_v3(l_ezlopi_item_t *item);
static int dht22_sensor_get_sensor_value_v3(l_ezlopi_item_t *item, void *args);
static int dht11_sensor_setup_item_properties_temperature(l_ezlopi_item_t *item, cJSON *cj_device, void *user_arg);
static int dht11_sensor_setup_item_properties_humidity(l_ezlopi_item_t *item, cJSON *cj_device, void *user_arg);
// static int dht22_sensor_setup_device_cloud_properties_humidity(l_ezlopi_device_t *device, cJSON *cj_device);
static int dht22_sensor_setup_device_cloud_properties_temperature(l_ezlopi_device_t *device, cJSON *cj_device);

static int __notify(l_ezlopi_item_t *item);

int sensor_0067_oneWire_DHT22(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg)
{
    TRACE_B("Action: %s", ezlopi_actions_to_string(action));
    int ret = 0;
    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        dht22_sensor_prepare_v3(arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        dht22_sensor_init_v3(item);
        break;
    }
    case EZLOPI_ACTION_HUB_GET_ITEM:
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        dht22_sensor_get_sensor_value_v3(item, arg);
        break;
    }
    case EZLOPI_ACTION_NOTIFY_1000_MS:
    {
        __notify(item);
        break;
    }
    default:
    {
        break;
    }
    }
    return ret;
}

static int dht22_sensor_init_v3(l_ezlopi_item_t *item)
{
    int ret = 0;

    // setDHTgpio(item->interface.onewire_master.onewire_pin);
    dht21_init(item->interface.onewire_master.onewire_pin);

    return ret;
}

static int __notify(l_ezlopi_item_t *item)
{
    int ret = 0;
    if (item)
    {
        s_ezlopi_dht22_data_t2 *dht22_data = (s_ezlopi_dht22_data_t2 *)item->user_arg;
        if (dht22_data)
        {

            if (ezlopi_item_name_temp == item->cloud_properties.item_name)
            {
            //@ my code
            dht21_poll_data(item->interface.onewire_master.onewire_pin, &sensor_data);
            if (dht21_checksum_ok(&sensor_data)) {
			temp_in_float = (float)dht21_temperature(&sensor_data)/10;
			humidity_in_float = (float)dht21_humidity(&sensor_data)/10;
			LOG("temp = %.2f, hum = %.2f", temp_in_float, humidity_in_float);
            
		    }
		    memset(&sensor_data, 0, sizeof(sensor_data));
            //@ end my code
                
                float temperature = temp_in_float;
                // TRACE_E("Temperature: %.2f", temperature);
                if (fabs(dht22_data->temperature - temperature) > 0.5)
                {
                    dht22_data->temperature = temperature;
                    ezlopi_device_value_updated_from_device_v3(item);
                }
                
            }
            else if (ezlopi_item_name_humidity == item->cloud_properties.item_name)
            {
                float humidity = humidity_in_float;
                // TRACE_E("Humidity: %.2f", humidity);
                if (fabs(dht22_data->humidity - humidity) > 0.5)
                {
                    dht22_data->humidity = humidity;
                    ezlopi_device_value_updated_from_device_v3(item);
                }
            }
            else
            {
            }
        }
    }
    return ret;
}
static int dht22_sensor_get_sensor_value_v3(l_ezlopi_item_t *item, void *args)
{
    int ret = 0;
    cJSON *cj_properties = (cJSON *)args;
    if (item && cj_properties)
    {

        s_ezlopi_dht22_data_t2 *dht22_data = (s_ezlopi_dht22_data_t2 *)item->user_arg;

        if (ezlopi_item_name_temp == item->cloud_properties.item_name)
        {

            cJSON_AddNumberToObject(cj_properties, "value", dht22_data->temperature); 
            char *valueFormatted = ezlopi_valueformatter_float(dht22_data->temperature);
            cJSON_AddStringToObject(cj_properties, "valueFormatted", valueFormatted);
            free(valueFormatted);
            cJSON_AddStringToObject(cj_properties, "scale", "celsius");
        }

        if (ezlopi_item_name_humidity == item->cloud_properties.item_name)
        {
            cJSON_AddNumberToObject(cj_properties, "value", dht22_data->humidity); 
            char *valueFormatted = ezlopi_valueformatter_float(dht22_data->humidity);
            cJSON_AddStringToObject(cj_properties, "valueFormatted", valueFormatted);
            free(valueFormatted);
            cJSON_AddStringToObject(cj_properties, "scale", "percent");
        }
    }
    return ret;
}

static int dht22_sensor_prepare_v3(void *arg)
{
    int ret = 0;
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg)
    {
        cJSON *cjson_device = prep_arg->cjson_device;
        if (cjson_device)
        {
            l_ezlopi_device_t *parent_device = ezlopi_device_add_device();
            if (parent_device)
            {
                s_ezlopi_dht22_data_t2 *dht22_sensor_data = (s_ezlopi_dht22_data_t2 *)malloc(sizeof(s_ezlopi_dht22_data_t2));

                if (dht22_sensor_data)
                {

                    dht22_sensor_setup_device_cloud_properties_temperature(parent_device, cjson_device);

                    l_ezlopi_item_t *item_temperature = ezlopi_device_add_item_to_device(parent_device, sensor_0067_oneWire_DHT22);
                    if (item_temperature)
                    {
                        dht11_sensor_setup_item_properties_temperature(item_temperature, cjson_device, dht22_sensor_data);
                    }

                    l_ezlopi_item_t *item_humidity = ezlopi_device_add_item_to_device(parent_device, sensor_0067_oneWire_DHT22);
                    if (item_humidity)
                    {

                        dht11_sensor_setup_item_properties_humidity(item_humidity, cjson_device, dht22_sensor_data);
                    }
                }
            }
        }
    }

    return ret;
}

static int dht22_sensor_setup_device_cloud_properties_temperature(l_ezlopi_device_t *device, cJSON *cj_device)
{
    int ret = 0;
    if (device && cj_device)
    {
        char *device_name = NULL;
        CJSON_GET_VALUE_STRING(cj_device, "dev_name", device_name);

        ASSIGN_DEVICE_NAME_V2(device, device_name);
        device->cloud_properties.category = category_temperature;
        device->cloud_properties.subcategory = subcategory_not_defined;
        device->cloud_properties.device_type = dev_type_sensor;
        device->cloud_properties.device_id = ezlopi_cloud_generate_device_id();
    }
    return ret;
}

static int dht11_sensor_setup_item_properties_temperature(l_ezlopi_item_t *item, cJSON *cj_device, void *user_arg)
{
    int ret = 0;

    if (item && cj_device)
    {
        item->cloud_properties.show = true;
        item->cloud_properties.has_getter = true;
        item->cloud_properties.has_setter = false;
        item->cloud_properties.item_name = ezlopi_item_name_temp;
        item->cloud_properties.value_type = value_type_temperature;
        item->cloud_properties.scale = scales_celsius;
        item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
        item->user_arg = user_arg;

        CJSON_GET_VALUE_INT(cj_device, "dev_type", item->interface_type);

        item->interface.onewire_master.enable = true;
        CJSON_GET_VALUE_INT(cj_device, "gpio", item->interface.onewire_master.onewire_pin);
    }

    return ret;
}

static int dht11_sensor_setup_item_properties_humidity(l_ezlopi_item_t *item, cJSON *cj_device, void *user_arg)
{
    int ret = 0;

    if (item && cj_device)
    {
        item->cloud_properties.show = true;
        item->cloud_properties.has_getter = true;
        item->cloud_properties.has_setter = false;
        item->cloud_properties.item_name = ezlopi_item_name_humidity;
        item->cloud_properties.value_type = value_type_humidity;
        item->cloud_properties.scale = scales_percent;
        item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
        item->user_arg = user_arg;

        CJSON_GET_VALUE_INT(cj_device, "dev_type", item->interface_type);

        item->interface.onewire_master.enable = true;
        CJSON_GET_VALUE_INT(cj_device, "gpio", item->interface.onewire_master.onewire_pin);
    }

    return ret;
}
