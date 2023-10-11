
#include "cJSON.h"
#include "trace.h"
#include "ezlopi_actions.h"
#include "ezlopi_timer.h"
#include "items.h"

#include "ezlopi_cloud.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_device_value_updated.h"
#include "ezlopi_cloud_constants.h"
#include "math.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"

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
static s_ezlopi_device_properties_t *motor_driver_a4988_switch_properties = NULL;
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
        // ret = ezlopi_motor_driver_a4988_init(ezlo_device);
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
        ADD_PROPERTIES_DEVICE_LIST(motor_driver_a4988_switch_properties, device_id, category_switch, subcategory_relay, ezlopi_item_name_switch, value_type_bool, prep_arg->cjson_device);
    }

    return ret;
}

static int ezlopi_motor_driver_a4988_init(s_ezlopi_device_properties_t *properties)
{
    int ret = -1;
    // static s_ezlopi_channel_speed_t *ezlopi_servo_motor_MG_996R_speed = NULL;
    // if (GPIO_IS_VALID_GPIO(properties->interface.pwm.gpio_num))
    // {
    //     ezlopi_servo_motor_MG_996R_speed = ezlopi_pwm_init(properties->interface.pwm.gpio_num, properties->interface.pwm.pwm_resln,
    //                                                        properties->interface.pwm.freq_hz, properties->interface.pwm.duty_cycle);
    //     properties->interface.pwm.channel = ezlopi_servo_motor_MG_996R_speed->channel;
    //     properties->interface.pwm.speed_mode = ezlopi_servo_motor_MG_996R_speed->speed_mode;
    //     ret = 0;
    // }
    
    // Configure MCPWM
    ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, properties->interface.pwm.gpio_num));
    mcpwm_config_t pwm_config = {
        .frequency = MG996R_SERVO_FREQUENCY,
        .cmpr_a = 0,
        .cmpr_b = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0
    };
    ESP_ERROR_CHECK(mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config));
    
    ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0));

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
        }
        if (ezlopi_item_name_switch == properties->ezlopi_cloud.item_name)
        {
            en_pin_state = value;
            TRACE_I("en_pin_status updated value: %d", value);
        }

        ezlopi_device_value_updated_from_device(properties);
    }
    return ret;
}

static int ezlopi_motor_driver_a4988_get_value_cjson(s_ezlopi_device_properties_t *properties, void *args)
{
    int ret = 0;
    cJSON *cjson_properties = (cJSON *)args;
    if (cjson_properties)
    {
        if (ezlopi_item_name_rotation == properties->ezlopi_cloud.item_name)
        {
            cJSON_AddNumberToObject(cjson_properties, "value", rotation);
            cJSON_AddStringToObject(cjson_properties, "scale", "revolutions_per_minute");
        }
        if (ezlopi_item_name_switch == properties->ezlopi_cloud.item_name)
        {
            cJSON_AddNumberToObject(cjson_properties, "value", en_pin_state);
        }
        ret = 1;
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
            // if (ezlopi_item_name_switch == item_name)
            // {
            //     ezlopi_motor_driver_a4988_properties->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_INPUT;
            //     ezlopi_motor_driver_a4988_properties->interface.gpio.gpio_in.enable = true;
            //     CJSON_GET_VALUE_INT(cjson_device, "gpio", ezlopi_motor_driver_a4988_properties->interface.gpio.gpio_in.gpio_num);
            //     ezlopi_motor_driver_a4988_properties->interface.gpio.gpio_in.interrupt = false;
            //     ezlopi_motor_driver_a4988_properties->interface.gpio.gpio_in.invert = false;
            //     ezlopi_motor_driver_a4988_properties->interface.gpio.gpio_in.mode = GPIO_MODE_INPUT;
            //     ezlopi_motor_driver_a4988_properties->interface.gpio.gpio_in.pull = false;
            //     ezlopi_motor_driver_a4988_properties->interface.gpio.gpio_in.value = false;
            //     ezlopi_motor_driver_a4988_properties->interface.gpio.gpio_out.enable = false;
            // }
            // else
            // {
            //     ezlopi_motor_driver_a4988_properties->interface_type = EZLOPI_DEVICE_INTERFACE_PWM;
            //     CJSON_GET_VALUE_INT(cjson_device, "gpio", ezlopi_motor_driver_a4988_properties->interface.pwm.gpio_num);
            //     CJSON_GET_VALUE_INT(cjson_device, "duty_cycle", ezlopi_motor_driver_a4988_properties->interface.pwm.duty_cycle);
            //     CJSON_GET_VALUE_INT(cjson_device, "freq_hz", ezlopi_motor_driver_a4988_properties->interface.pwm.freq_hz);
            //     ezlopi_motor_driver_a4988_properties->interface.pwm.pwm_resln = 12;
            // }
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
