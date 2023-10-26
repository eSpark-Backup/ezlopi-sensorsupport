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
#include "driver/keypad.h"
#include "sensor_0066_matrix_keypad.h"
#include <math.h>

static bool matrix_keypad_initialized = false;
keypad_config_t matrix_keypad_config;
static int counter = 5;

static int matrix_keypad_sensor_prepare(void *args);
static int matrix_keypad_sensor_add_to_list(s_ezlopi_prep_arg_t *prep_arg, s_ezlopi_device_properties_t *matrix_keypad_sensor_properties, void *user_arg);
static s_ezlopi_device_properties_t *matrix_keypad_sensor_prepare_properties(uint32_t device_id, const char *category, const char *sub_category, const char *item_name, const char *value_type, cJSON *cjson_device);
static int matrix_keypad_sensor_init(s_ezlopi_device_properties_t *properties);
static int matrix_keypad_sensor_get_sensor_value(s_ezlopi_device_properties_t *properties, void *args);

#define ADD_PROPERTIES_DEVICE_LIST(device_id, category, sub_category, item_name, value_type, cjson_device)                \
    {                                                                                                                     \
        s_ezlopi_device_properties_t *_properties = matrix_keypad_sensor_prepare_properties(device_id, category, sub_category,  \
                                                                                    item_name, value_type, cjson_device); \
        if (NULL != _properties)                                                                                          \
        {                                                                                                                 \
            matrix_keypad_sensor_add_to_list(prep_arg, _properties, NULL);                                                      \
        }                                                                                                                 \
    }

int sensor_0066_matrix_keypad(e_ezlopi_actions_t action, s_ezlopi_device_properties_t *ezlo_device, void *arg, void *user_args)
{
    int ret = 0;
    switch (action)
    {
        case EZLOPI_ACTION_PREPARE:
        {
            matrix_keypad_sensor_prepare(arg);
            break;
        }
        case EZLOPI_ACTION_INITIALIZE:
        {
            matrix_keypad_sensor_init(ezlo_device);
            break;
        }
        case EZLOPI_ACTION_HUB_GET_ITEM:
        case EZLOPI_ACTION_GET_EZLOPI_VALUE:
        {
            matrix_keypad_sensor_get_sensor_value(ezlo_device, arg);
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

static int matrix_keypad_sensor_prepare(void *arg)
{
    int ret = 0;
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;

    if ((NULL != prep_arg) && (NULL != prep_arg->cjson_device))
    {
        uint32_t device_id = ezlopi_cloud_generate_device_id();
        // voltage
        ADD_PROPERTIES_DEVICE_LIST(device_id, category_keypad, subcategory_not_defined, ezlopi_item_name_master_code, value_type_string, prep_arg->cjson_device);
    }
    return ret;
}

static s_ezlopi_device_properties_t *matrix_keypad_sensor_prepare_properties(uint32_t device_id, const char *category, const char *sub_category, const char *item_name, const char *value_type, cJSON *cjson_device)
{
    s_ezlopi_device_properties_t *matrix_keypad_sensor_properties = NULL;

    if (NULL != cjson_device)
    {
        matrix_keypad_sensor_properties = (s_ezlopi_device_properties_t *)malloc(sizeof(s_ezlopi_device_properties_t));
        if (matrix_keypad_sensor_properties)
        {
            memset(matrix_keypad_sensor_properties, 0, sizeof(s_ezlopi_device_properties_t));
            matrix_keypad_sensor_properties->interface_type = EZLOPI_DEVICE_INTERFACE_ONEWIRE_MASTER;

            char *device_name = NULL;

            CJSON_GET_VALUE_STRING(cjson_device, "dev_name", device_name);
            ASSIGN_DEVICE_NAME(matrix_keypad_sensor_properties, device_name);
            matrix_keypad_sensor_properties->ezlopi_cloud.category = category;
            matrix_keypad_sensor_properties->ezlopi_cloud.subcategory = sub_category;
            matrix_keypad_sensor_properties->ezlopi_cloud.item_name = item_name;
            matrix_keypad_sensor_properties->ezlopi_cloud.device_type = dev_type_sensor;
            matrix_keypad_sensor_properties->ezlopi_cloud.value_type = value_type;
            matrix_keypad_sensor_properties->ezlopi_cloud.reachable = true;
            matrix_keypad_sensor_properties->ezlopi_cloud.battery_powered = false;
            matrix_keypad_sensor_properties->ezlopi_cloud.has_getter = true;
            matrix_keypad_sensor_properties->ezlopi_cloud.has_setter = true;
            matrix_keypad_sensor_properties->ezlopi_cloud.show = true;
            matrix_keypad_sensor_properties->ezlopi_cloud.room_name[0] = '\0';
            matrix_keypad_sensor_properties->ezlopi_cloud.device_id = device_id;
            matrix_keypad_sensor_properties->ezlopi_cloud.room_id = ezlopi_cloud_generate_room_id();
            matrix_keypad_sensor_properties->ezlopi_cloud.item_id = ezlopi_cloud_generate_item_id();

            CJSON_GET_VALUE_INT(cjson_device, "rows", matrix_keypad_config.rows);
            CJSON_GET_VALUE_INT(cjson_device, "cols", matrix_keypad_config.cols);

            cJSON *gpio_array = cJSON_GetObjectItem(cjson_device, "gpio_array");       
            int gpio_array_size = cJSON_GetArraySize(gpio_array);
            matrix_keypad_config.gpios = (gpio_num_t *)malloc(gpio_array_size * sizeof(gpio_num_t));

            if (NULL != matrix_keypad_config.gpios)
            {
                for (int i = 0; i < gpio_array_size; i++) 
                {
                    cJSON *gpio_item = cJSON_GetArrayItem(gpio_array, i);
                    matrix_keypad_config.gpios[i] = gpio_item->valueint;
                    TRACE_I("gpio[%d] : %d ", i, (gpio_num_t)gpio_item->valueint);
                }
            }
        }
    }
    return matrix_keypad_sensor_properties;
}

static int matrix_keypad_sensor_add_to_list(s_ezlopi_prep_arg_t *prep_arg, s_ezlopi_device_properties_t *matrix_keypad_sensor_properties, void *user_arg)
{
    int ret = 0;
    if (matrix_keypad_sensor_properties)
    {
        if (0 == ezlopi_devices_list_add(prep_arg->device, matrix_keypad_sensor_properties, user_arg))
        {
            free(matrix_keypad_sensor_properties);
        }
        else
        {
            ret = 1;
        }
    }
    return ret;
}

static int matrix_keypad_sensor_init(s_ezlopi_device_properties_t *properties)
{
    int ret = 0;

    if (!matrix_keypad_initialized)
    {
        /// Initialize keyboard
        keypad_initalize(matrix_keypad_config.gpios, matrix_keypad_config.rows, matrix_keypad_config.cols);
        matrix_keypad_initialized = true;
    }

    return ret;
}

static int matrix_keypad_sensor_get_sensor_value(s_ezlopi_device_properties_t *properties, void *args)
{
    int ret = 0;

    cJSON *cjson_properties = (cJSON *)args;
    char formatted_value[20];

    char key[20];
    uint8_t i = 0;
    do
    {
        key[i] = keypad_getkey();
        /* code */
    } while ('\0' != key[i++]);
    key[i] = '\0';

    if (cjson_properties)
    {
        TRACE_I("Code %s", key);
        cJSON_AddStringToObject(cjson_properties, "value", key);

    }

    return ret;
}
