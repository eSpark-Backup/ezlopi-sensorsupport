
#include "cJSON.h"
#include "ezlopi_cloud.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_device_value_updated.h"
#include "ezlopi_cloud_category_str.h"
#include "ezlopi_cloud_subcategory_str.h"
#include "ezlopi_item_name_str.h"
#include "ezlopi_cloud_device_types_str.h"
#include "ezlopi_cloud_value_type_str.h"
#include "trace.h"
#include "driver/ina219.h"
#include "sensor_0065_i2c_sen0291_digital_wattmeter.h"
#include <math.h>

static bool sen0291_initialized = false;
static int counter = 5;

static int sen0291_sensor_prepare(void *args);
static int sen0291_sensor_add_to_list(s_ezlopi_prep_arg_t *prep_arg, s_ezlopi_device_properties_t *sen0291_sensor_properties, void *user_arg);
static s_ezlopi_device_properties_t *sen0291_sensor_prepare_properties(uint32_t device_id, const char *category, const char *sub_category, const char *item_name, const char *value_type, cJSON *cjson_device);
static int sen0291_sensor_init(s_ezlopi_device_properties_t *properties);
static int sen0291_sensor_get_sensor_value(s_ezlopi_device_properties_t *properties, void *args);

#define ADD_PROPERTIES_DEVICE_LIST(device_id, category, sub_category, item_name, value_type, cjson_device)                \
    {                                                                                                                     \
        s_ezlopi_device_properties_t *_properties = sen0291_sensor_prepare_properties(device_id, category, sub_category,  \
                                                                                    item_name, value_type, cjson_device); \
        if (NULL != _properties)                                                                                          \
        {                                                                                                                 \
            sen0291_sensor_add_to_list(prep_arg, _properties, NULL);                                                      \
        }                                                                                                                 \
    }

int sensor_0065_i2c_sen0291_digital_wattmeter(e_ezlopi_actions_t action, s_ezlopi_device_properties_t *ezlo_device, void *arg, void *user_args)
{
    int ret = 0;
    switch (action)
    {
        case EZLOPI_ACTION_PREPARE:
        {
            sen0291_sensor_prepare(arg);
            break;
        }
        case EZLOPI_ACTION_INITIALIZE:
        {
            sen0291_sensor_init(ezlo_device);
            break;
        }
        case EZLOPI_ACTION_HUB_GET_ITEM:
        case EZLOPI_ACTION_GET_EZLOPI_VALUE:
        {
            sen0291_sensor_get_sensor_value(ezlo_device, arg);
            break;
        }
        case EZLOPI_ACTION_NOTIFY_1000_MS:
        {
            if (5 == counter)
            {
                ezlopi_device_value_updated_from_device(ezlo_device);
                counter = 0;
            }
            counter++;
            break;
        }
        default:
        {
            break;
        }
    }
    return ret;
}

static int sen0291_sensor_prepare(void *arg)
{
    int ret = 0;
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;

    if ((NULL != prep_arg) && (NULL != prep_arg->cjson_device))
    {
        uint32_t device_id = ezlopi_cloud_generate_device_id();
        // voltage
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_level_sensor, subcategory_electricity, ezlopi_item_name_voltage, value_type_electric_potential, prep_arg->cjson_device);
        // current
        device_id = ezlopi_cloud_generate_device_id();
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_level_sensor, subcategory_electricity, ezlopi_item_name_current, value_type_electric_current, prep_arg->cjson_device);
        // power
        device_id = ezlopi_cloud_generate_device_id();
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_level_sensor, subcategory_electricity, ezlopi_item_name_power, value_type_power, prep_arg->cjson_device);
    }
    return ret;
}

static s_ezlopi_device_properties_t *sen0291_sensor_prepare_properties(uint32_t device_id, const char *category, const char *sub_category, const char *item_name, const char *value_type, cJSON *cjson_device)
{
    s_ezlopi_device_properties_t *sen0291_sensor_properties = NULL;

    if (NULL != cjson_device)
    {
        sen0291_sensor_properties = (s_ezlopi_device_properties_t *)malloc(sizeof(s_ezlopi_device_properties_t));
        if (sen0291_sensor_properties)
        {
            memset(sen0291_sensor_properties, 0, sizeof(s_ezlopi_device_properties_t));
            sen0291_sensor_properties->interface_type = EZLOPI_DEVICE_INTERFACE_ONEWIRE_MASTER;

            char *device_name = NULL;

            CJSON_GET_VALUE_STRING(cjson_device, "dev_name", device_name);
            ASSIGN_DEVICE_NAME(sen0291_sensor_properties, device_name);
            sen0291_sensor_properties->ezlopi_cloud.category = category;
            sen0291_sensor_properties->ezlopi_cloud.subcategory = sub_category;
            sen0291_sensor_properties->ezlopi_cloud.item_name = item_name;
            sen0291_sensor_properties->ezlopi_cloud.device_type = dev_type_sensor;
            sen0291_sensor_properties->ezlopi_cloud.value_type = value_type;
            sen0291_sensor_properties->ezlopi_cloud.reachable = true;
            sen0291_sensor_properties->ezlopi_cloud.battery_powered = false;
            sen0291_sensor_properties->ezlopi_cloud.has_getter = true;
            sen0291_sensor_properties->ezlopi_cloud.show = true;
            sen0291_sensor_properties->ezlopi_cloud.room_name[0] = '\0';
            sen0291_sensor_properties->ezlopi_cloud.device_id = device_id;
            sen0291_sensor_properties->ezlopi_cloud.room_id = ezlopi_cloud_generate_room_id();
            sen0291_sensor_properties->ezlopi_cloud.item_id = ezlopi_cloud_generate_item_id();
            CJSON_GET_VALUE_INT(cjson_device, "gpio_sda", sen0291_sensor_properties->interface.i2c_master.sda);
            CJSON_GET_VALUE_INT(cjson_device, "gpio_scl", sen0291_sensor_properties->interface.i2c_master.scl);
            CJSON_GET_VALUE_INT(cjson_device, "i2c_address", sen0291_sensor_properties->interface.i2c_master.address);
            if (!sen0291_sensor_properties->interface.i2c_master.address)
            {
                sen0291_sensor_properties->interface.i2c_master.address = INA219_ADDR_VS_VS;
            }
            sen0291_sensor_properties->interface.i2c_master.enable = true;
            sen0291_sensor_properties->interface.i2c_master.channel = I2C_NUM_0;
            sen0291_sensor_properties->interface.i2c_master.clock_speed = 400000;
            sen0291_sensor_properties->interface_type = EZLOPI_DEVICE_INTERFACE_I2C_MASTER;
            

            if(ezlopi_item_name_voltage == item_name)
            {
                strcat(sen0291_sensor_properties->ezlopi_cloud.device_name, " Voltage");
            }
            else if(ezlopi_item_name_current == item_name)
            {
                strcat(sen0291_sensor_properties->ezlopi_cloud.device_name, " Current");
            }
            else if(ezlopi_item_name_power == item_name)
            {
                strcat(sen0291_sensor_properties->ezlopi_cloud.device_name, " Power");
            }
        }
    }
    return sen0291_sensor_properties;
}

static int sen0291_sensor_add_to_list(s_ezlopi_prep_arg_t *prep_arg, s_ezlopi_device_properties_t *sen0291_sensor_properties, void *user_arg)
{
    int ret = 0;
    if (sen0291_sensor_properties)
    {
        if (0 == ezlopi_devices_list_add(prep_arg->device, sen0291_sensor_properties, user_arg))
        {
            free(sen0291_sensor_properties);
        }
        else
        {
            ret = 1;
        }
    }
    return ret;
}

static int sen0291_sensor_init(s_ezlopi_device_properties_t *properties)
{
    int ret = 0;

    if (!sen0291_initialized)
    {
        ina219_digital_wattmeter_setup(&properties->interface.i2c_master);
        sen0291_initialized = true;
    }

    return ret;
}

static int sen0291_sensor_get_sensor_value(s_ezlopi_device_properties_t *properties, void *args)
{
    int ret = 0;

    cJSON *cjson_properties = (cJSON *)args;
    char formatted_value[20];

    digital_wattmeter_data_t sen0291_data;
    ina219_digital_wattmeter_get_data(&sen0291_data);

    if (cjson_properties)
    {
        if (ezlopi_item_name_voltage == properties->ezlopi_cloud.item_name)
        {
            sen0291_data.bus_voltage *= 1000; // Volts to milli volts
            TRACE_I("voltage : %.2f mV", sen0291_data.bus_voltage);
            snprintf(formatted_value, 20, "%0.2f", sen0291_data.bus_voltage);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            cJSON_AddNumberToObject(cjson_properties, "value", atof(formatted_value));
            cJSON_AddStringToObject(cjson_properties, "scale", "volt");
        }
        else if (ezlopi_item_name_current == properties->ezlopi_cloud.item_name)
        {
            sen0291_data.current *= 1000; // Amps to milli amps
            TRACE_I("current: %.2f mA", sen0291_data.current);
            snprintf(formatted_value, 20, "%0.2f", sen0291_data.current);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            cJSON_AddNumberToObject(cjson_properties, "value", atof(formatted_value));
            cJSON_AddStringToObject(cjson_properties, "scale", "milli_ampere");
        }
        else if (ezlopi_item_name_power == properties->ezlopi_cloud.item_name)
        {
            TRACE_I("power: %.2f W", sen0291_data.power);
            snprintf(formatted_value, 20, "%0.2f", sen0291_data.power);
            cJSON_AddStringToObject(cjson_properties, "valueFormatted", formatted_value);
            cJSON_AddNumberToObject(cjson_properties, "value", atof(formatted_value));
            cJSON_AddStringToObject(cjson_properties, "scale", "watt");
        }
    }

    return ret;
}
