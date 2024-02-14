#include "cJSON.h"
#include "ezlopi_util_trace.h"

// #include "ezlopi_actions.h"
#include "ezlopi_core_actions.h"
#include "ezlopi_core_timer.h"
#include "ezlopi_cloud_items.h"

#include "ezlopi_core_cloud.h"
#include "ezlopi_core_devices_list.h"
#include "ezlopi_core_device_value_updated.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_valueformatter.h"
#include "ezlopi_core_cjson_macros.h"

#include <sys/time.h>
#include "esp_sleep.h"
#include "esp_log.h"


#include "device_0069_DeepSleep.h"

// static bool deepsleep_initialized = false;
const char *deepsleep_trigger_token[] =
    {
        "10 min",//by default
        "20 min",
        "30 min",
        "40 min"
    };

static RTC_DATA_ATTR struct timeval sleep_enter_time;
deepsleep_parameters_t deepsleep_init_params;
static l_ezlopi_item_t *deepsleep_item = NULL;

static int __prepare(void *arg);
static int __init(l_ezlopi_item_t *item);
static int __get_cjson_value(l_ezlopi_item_t *item, void *arg);
static int __set_cjson_value(l_ezlopi_item_t *item, void *arg);
static void __setup_device_cloud_params(l_ezlopi_device_t *device, cJSON *cj_device);

int device_0069_DeepSleep(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void* user_arg)
{
    int ret = 0;

    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        ret = __prepare(arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        ret = __init(item);
        break;
    }
    case EZLOPI_ACTION_SET_VALUE:
    {
        ret = __set_cjson_value(item, arg);
        break;
    }
    case EZLOPI_ACTION_HUB_GET_ITEM:
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        ret = __get_cjson_value(item, arg);
        break;
    }
    default:
    {
        break;
    }
    }

    return ret;
}

static int __set_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;
    deepsleep_init_params.sleep_time_ms = (deepsleep_init_params.now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (deepsleep_init_params.now.tv_usec - sleep_enter_time.tv_usec) / 1000;
    cJSON *device_details = (cJSON *)arg;
    // ezlopi_deepsleep_action_struct_t *deepsleep_state = (ezlopi_deepsleep_action_struct_t *)item->user_arg;
    if (device_details)
    {
        if (ezlopi_item_name_energy_saving_mode == item->cloud_properties.item_name)
        {
            if(esp_sleep_get_wakeup_cause()==ESP_SLEEP_WAKEUP_TIMER)
            {
                ESP_LOGI("DEEPSLEEP","Wake up from timer. Time spent in deep sleep: %dms", deepsleep_init_params.sleep_time_ms);
            }
            else if(ESP_SLEEP_WAKEUP_UNDEFINED)
            {
                ESP_LOGI("DEEPSLEEP","Not a deep sleep reset");     
            }
            char *value = NULL;
            CJSON_GET_VALUE_STRING(device_details, "value", value);

            /* loop through each token and apply which is received */
            for (uint8_t i = 0; i < DEEPSLEEP_MAX_ENUMS; i++)
            {
                if(value!=NULL && !strcmp(value, deepsleep_trigger_token[i]))
                {
                    deepsleep_init_params.wakeup_time_sec = (i+1)*10;
                    esp_sleep_enable_timer_wakeup(deepsleep_init_params.wakeup_time_sec * 1000000);
                    gettimeofday(&sleep_enter_time, NULL);
                    ESP_LOGI("DEEPSLEEP","GOING TO SLEEP! GOODNIGHT!");
                    esp_deep_sleep_start();
                    break;
                }
            }
        }
    }

    return ret;
}

static int __get_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;

    cJSON *param = (cJSON *)arg;
    ezlopi_deepsleep_action_struct_t *deepsleep_state = (ezlopi_deepsleep_action_struct_t *)item->user_arg;
    if ((NULL != param) && (NULL != deepsleep_state))
    {
        ESP_LOGI("DEEPSLEEP","Here1");
        if (ezlopi_item_name_energy_saving_mode == item->cloud_properties.item_name)
        {
            ESP_LOGI("DEEPSLEEP","Here2");
            //-------------------  POSSIBLE JSON ENUM CONTENTS ----------------------------------
            cJSON *json_array_enum = cJSON_CreateArray();
            if (NULL != json_array_enum)
            {
                ESP_LOGI("DEEPSLEEP","Here3");
                for (uint8_t i = 0; i < DEEPSLEEP_MAX_ENUMS; i++)
                {
                    cJSON *json_value = cJSON_CreateString(deepsleep_trigger_token[i]);
                    ESP_LOGI("DEEPSLEEP","Here4");
                    if (NULL != json_value)
                    {
                        ESP_LOGI("DEEPSLEEP","Here5");
                        cJSON_AddItemToArray(json_array_enum, json_value);
                    }
                }
                cJSON_AddItemToObject(param, "enum", json_array_enum);
            }
            cJSON_AddStringToObject(param, "value", deepsleep_state->current_trigger_value);

        }
    }

    return ret;
}

static int __init(l_ezlopi_item_t *item)
{
    gettimeofday(&deepsleep_init_params.now,NULL);
    deepsleep_init_params.wakeup_time_sec = 10;
    return 1;
}

static int __prepare(void *arg)
{
    ESP_LOGI("DEEPSLEEP","Here");
    int ret = 0;
    ezlopi_deepsleep_action_struct_t *deepsleep_state = (ezlopi_deepsleep_action_struct_t*)malloc(sizeof(ezlopi_deepsleep_action_struct_t));
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg && prep_arg->cjson_device)
    {
        ESP_LOGI("DEEPSLEEP","Here1");
        cJSON *cj_device = prep_arg->cjson_device;
        l_ezlopi_device_t *device = ezlopi_device_add_device(cj_device);
        if (device)
        {
            ESP_LOGI("DEEPSLEEP","Here2");
            __setup_device_cloud_params(device, cj_device);
            deepsleep_item = ezlopi_device_add_item_to_device(device, device_0069_DeepSleep);
            if (deepsleep_item)
            {
                ESP_LOGI("DEEPSLEEP","Here3");
                deepsleep_item->cloud_properties.show = true;
                deepsleep_item->cloud_properties.has_getter = true;
                deepsleep_item->cloud_properties.has_setter = true;
                deepsleep_item->cloud_properties.item_name = ezlopi_item_name_energy_saving_mode;
                deepsleep_item->cloud_properties.value_type = value_type_token;
                deepsleep_item->cloud_properties.scale = NULL;
                deepsleep_item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
                /* initialize rf structure */
                deepsleep_state->current_trigger_value = "10 min";
                deepsleep_item->user_arg = deepsleep_state;
                ESP_LOGI("DEEPSLEEP","Here4");
            }
        }
    }

    return ret;
}

static void __setup_device_cloud_params(l_ezlopi_device_t *device, cJSON *cj_device)
{
    char *device_name = NULL;
    CJSON_GET_VALUE_STRING(cj_device, "dev_name", device_name);

    ASSIGN_DEVICE_NAME_V2(device, device_name);
    device->cloud_properties.category = category_remote_control;
    device->cloud_properties.subcategory = subcategory_not_defined;
    device->cloud_properties.device_type = dev_type_device;
    device->cloud_properties.device_id = ezlopi_cloud_generate_device_id();
}