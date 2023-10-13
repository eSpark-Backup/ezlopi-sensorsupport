
#include "cJSON.h"
#include "trace.h"
#include "ezlopi_actions.h"
#include "ezlopi_timer.h"
#include "items.h"
#include "ezlopi_cloud.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_device_value_updated.h"
#include "ezlopi_cloud_constants.h"
#include "a4988.h"
#include "device_0064_motor_driver_a4988.h"

static int ezlopi_motor_driver_a4988_prepare(void *args);
static int ezlopi_motor_driver_a4988_init(s_ezlopi_device_properties_t *properties);
static int ezlopi_motor_driver_a4988_get_value_cjson(s_ezlopi_device_properties_t *properties, void *args);
static int ezlopi_motor_driver_a4988_set_value(s_ezlopi_device_properties_t *properties, void *arg);

static s_ezlopi_device_properties_t *ezlopi_motor_driver_a4988_prepare_properties(uint32_t device_id, const char *category,
                                                                      const char *sub_category, const char *item_name,
                                                                      const char *value_type, cJSON *cjson_device);
static int add_device_to_list(s_ezlopi_prep_arg_t *prep_arg, s_ezlopi_device_properties_t *properties, void *user_args);

static s_ezlopi_device_properties_t *motor_driver_a4988_rotation_properties = NULL;
static s_ezlopi_device_properties_t *motor_driver_a4988_switch_enable_properties = NULL;
static s_ezlopi_device_properties_t *motor_driver_a4988_switch_direction_properties = NULL;
static a4988_config_t* a4988_config = NULL;
int rotation = 0;
bool en_pin_state = 0;

#define ADD_PROPERTIES_DEVICE_LIST(_properties, device_id, category, sub_category, item_name, value_type, cjson_device)                       \
    {                                                                                                                                         \
        _properties = ezlopi_motor_driver_a4988_prepare_properties(device_id, category, sub_category, item_name, value_type, cjson_device);   \
        if (NULL != _properties)                                                                                                              \
        {                                                                                                                                     \
            add_device_to_list(prep_arg, _properties, NULL);                                                                                  \
        }                                                                                                                                     \
    }

int device_0064_motor_driver_a4988(e_ezlopi_actions_t action, s_ezlopi_device_properties_t *ezlo_device, void *arg, void *user_arg)
{
    int ret = 0;
    switch (action)
    {
        case EZLOPI_ACTION_PREPARE:
        {
            ret = ezlopi_motor_driver_a4988_prepare(arg);
            break;
        }
        case EZLOPI_ACTION_INITIALIZE:
        {
            ret = ezlopi_motor_driver_a4988_init(ezlo_device);
            break;
        }
        case EZLOPI_ACTION_SET_VALUE:
        {
            ret = ezlopi_motor_driver_a4988_set_value(ezlo_device, arg);
            break;
        }
        case EZLOPI_ACTION_GET_EZLOPI_VALUE:
        {
            ret = ezlopi_motor_driver_a4988_get_value_cjson(ezlo_device, arg);
            break;
        }
        default:
        {
            break;
        }
    }

    return ret;
}

static int ezlopi_motor_driver_a4988_prepare(void *args)
{
    int ret = 0;
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)args;

    if ((NULL != prep_arg) && (NULL != prep_arg->cjson_device))
    {
        uint32_t device_id = ezlopi_cloud_generate_device_id();

        ADD_PROPERTIES_DEVICE_LIST(motor_driver_a4988_rotation_properties, device_id, category_interface, subcategory_not_defined, ezlopi_item_name_rotation, value_type_frequency, prep_arg->cjson_device);
        ADD_PROPERTIES_DEVICE_LIST(motor_driver_a4988_switch_enable_properties, device_id, category_switch, subcategory_relay, ezlopi_item_name_switch, value_type_bool, prep_arg->cjson_device);
        ADD_PROPERTIES_DEVICE_LIST(motor_driver_a4988_switch_direction_properties, device_id, category_switch, subcategory_relay, ezlopi_item_name_switch, value_type_bool, prep_arg->cjson_device);
    }

    return ret;
}

static int ezlopi_motor_driver_a4988_init(s_ezlopi_device_properties_t *properties)
{
    int ret = -1;

    if (a4988_config) 
    {
        a4988_config->en_pin = motor_driver_a4988_switch_enable_properties->interface.gpio.gpio_out.gpio_num;
        a4988_config->step_pin = motor_driver_a4988_rotation_properties->interface.gpio.gpio_out.gpio_num;
        a4988_config->direction_pin = motor_driver_a4988_switch_direction_properties->interface.gpio.gpio_out.gpio_num;

        ret = a4988_setup(a4988_config);
        a4988_disable_driver();
        
        free(a4988_config);
        a4988_config = NULL;
    }

    return ret;
}

static int ezlopi_motor_driver_a4988_set_value(s_ezlopi_device_properties_t *properties, void *arg)
{
    int ret = 0;
    cJSON *cjson_params = (cJSON *)arg;

    if (NULL != cjson_params)
    {
        int value = 0;
        CJSON_GET_VALUE_INT(cjson_params, "value", value);
        TRACE_I("cur value: %d", value);

        if (ezlopi_item_name_rotation == properties->ezlopi_cloud.item_name)
        {
            rotation = value;
            TRACE_I("rotation update value: %d", value);
            a4988_set_speed(value);
        }
        else if (ezlopi_item_name_switch == properties->ezlopi_cloud.item_name)
        {
            if (strstr(properties->ezlopi_cloud.device_name, "enable"))
            {
                if(value) 
                    a4988_enable_driver();
                else 
                    a4988_disable_driver();
                TRACE_I("Enable updated value: %d", value);

            }
            if (strstr(properties->ezlopi_cloud.device_name, "direction"))
            {
                a4988_set_direction(value);
                TRACE_I("Direction updated value: %d", value);
            }
            
        }

        // ezlopi_device_value_updated_from_device(properties);
    }
    return ret;
}

static int ezlopi_motor_driver_a4988_get_value_cjson(s_ezlopi_device_properties_t *properties, void *args)
{
    int ret = false;
    cJSON *cjson_properties = (cJSON *)args;
    if (cjson_properties)
    {
        if (ezlopi_item_name_rotation == properties->ezlopi_cloud.item_name)
        {
            cJSON_AddNumberToObject(cjson_properties, "value", rotation);
            cJSON_AddStringToObject(cjson_properties, "scale", "revolutions_per_minute");
        }
        else if (ezlopi_item_name_switch == properties->ezlopi_cloud.item_name )
        {
            bool gpio_level = gpio_get_level(properties->interface.gpio.gpio_out.gpio_num);
            if (properties->interface.gpio.gpio_out.invert)
                gpio_level = !gpio_level;

            cJSON_AddNumberToObject(cjson_properties, "value", gpio_level);  
            TRACE_E("gpio %d current value %d", properties->interface.gpio.gpio_out.gpio_num, gpio_level);        
        }

        ret = true;
    }
    return ret;
}

static s_ezlopi_device_properties_t *ezlopi_motor_driver_a4988_prepare_properties(uint32_t device_id, const char *category,
                                                                      const char *sub_category, const char *item_name,
                                                                      const char *value_type, cJSON *cjson_device)
{
    s_ezlopi_device_properties_t *ezlopi_motor_driver_a4988_properties = NULL;
    if (NULL != cjson_device)
    {
        ezlopi_motor_driver_a4988_properties = (s_ezlopi_device_properties_t *)malloc(sizeof(s_ezlopi_device_properties_t));

        if (ezlopi_motor_driver_a4988_properties)
        {
            memset(ezlopi_motor_driver_a4988_properties, 0, sizeof(s_ezlopi_device_properties_t));

            char *device_name = NULL;
            CJSON_GET_VALUE_STRING(cjson_device, "dev_name", device_name);
            ASSIGN_DEVICE_NAME(ezlopi_motor_driver_a4988_properties, device_name);
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.category = category;
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.subcategory = sub_category;
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.item_name = item_name;
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.value_type = value_type;
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.device_type = dev_type_device;
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.device_id = device_id;
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.item_id = ezlopi_cloud_generate_item_id();
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.room_id = ezlopi_cloud_generate_room_id();
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.reachable = true;
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.battery_powered = false;
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.show = true;
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.has_getter = true;
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.has_setter = true;
            ezlopi_motor_driver_a4988_properties->ezlopi_cloud.room_name[0] = '\0';
            int gpio_num = 0;
            if (ezlopi_item_name_switch == item_name)
            {
                if (NULL == motor_driver_a4988_switch_enable_properties)
                {
                    CJSON_GET_VALUE_INT(cjson_device, "gpio_en", gpio_num);
                    strcat(ezlopi_motor_driver_a4988_properties->ezlopi_cloud.device_name, " enable");
                    ezlopi_motor_driver_a4988_properties->interface.gpio.gpio_out.invert = 1;
                }
                else if (NULL == motor_driver_a4988_switch_direction_properties)
                {     
                    CJSON_GET_VALUE_INT(cjson_device, "gpio_dir", gpio_num);
                    strcat(ezlopi_motor_driver_a4988_properties->ezlopi_cloud.device_name, " direction");
                }
            }
            else if(ezlopi_item_name_rotation == item_name)
            {
                CJSON_GET_VALUE_INT(cjson_device, "gpio_step", gpio_num);
                strcat(ezlopi_motor_driver_a4988_properties->ezlopi_cloud.device_name, " speed");
                a4988_config = (a4988_config_t*)malloc(sizeof(a4988_config_t));
                if (a4988_config)
                {
                    CJSON_GET_VALUE_INT(cjson_device, "steps_per_rev", a4988_config->steps_per_rev);      
                    ezlopi_motor_driver_a4988_properties->user_arg = a4988_config;
                }
            }
            ezlopi_motor_driver_a4988_properties->interface.gpio.gpio_out.enable = true;
            ezlopi_motor_driver_a4988_properties->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT;
            ezlopi_motor_driver_a4988_properties->interface.gpio.gpio_out.gpio_num = gpio_num;
        }
    }
    return ezlopi_motor_driver_a4988_properties;
}

static int add_device_to_list(s_ezlopi_prep_arg_t *prep_arg, s_ezlopi_device_properties_t *properties, void *user_args)
{
    int ret = 0;

    if (properties)
    {
        if (0 == ezlopi_devices_list_add(prep_arg->device, properties, user_args))
        {
            free(properties);
        }
        else
        {
            ret = 1;
        }
    }
    return ret;
}
