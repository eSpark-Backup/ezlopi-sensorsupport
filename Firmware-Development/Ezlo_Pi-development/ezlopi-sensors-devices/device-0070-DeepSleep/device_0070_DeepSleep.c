#include "esp_log.h"
#include <sys/time.h>
#include "esp_sleep.h"
#include "trans_recv.h"

#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_settings.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_device_value_updated.h"

#include "device_0070_DeepSleep.h"

deepsleep_parameters_t deepsleep_init_params;

/*RF Control*/
static bool rf_module_initialized = false;
const char *rf_module_trigger_token[] =
    {
        "idle",
        "sleep",
        "shutdown",
        "unknown"
    };

static l_ezlopi_item_t *rf_module_item = NULL;

static int __prepare(void *arg);
static int __init(l_ezlopi_item_t *item);
static int __get_cjson_value(l_ezlopi_item_t *item, void *arg);
static int __set_cjson_value(l_ezlopi_item_t *item, void *arg);
static void __setup_device_cloud_params(l_ezlopi_device_t *device, cJSON *cj_device);
static int device_0070_DeepSleep_settings(e_ezlopi_settings_action_t action, l_ezlopi_device_settings_v3_t *setting, void *arg, void *user_arg);

static int device_0070_DeepSleep_settings(e_ezlopi_settings_action_t action, l_ezlopi_device_settings_v3_t *setting, void *arg, void *user_arg)
{
    int ret = 0;
    switch (action)
    {
    case EZLOPI_SETTINGS_ACTION_SET_SETTING:
    {
        cJSON* params = (cJSON*)arg;
        if(params)
        {
            int value = 0;
            char *value_string = 'unknown';
            CJSON_GET_VALUE_INT(params, "value", value);
            ESP_LOGI("DEEPSLEEP","Timer value from the cloud: %d",value);
            if(value == 0 && value_string == 'unknown')
            {
                CJSON_GET_VALUE_STRING(params, "value", value_string);
                ESP_LOGI("DEEPSLEEP","Trigger word from the cloud: %s",value_string);
            }

            if(strcmp(value_string, rf_module_trigger_token[1]) && strcmp(value_string, rf_module_trigger_token[2]))
            {
                setSyncWord(WAKE_UP_SYNC_WORD_BYTE, 145);
                esp_sleep_enable_ext0_wakeup(GPIO_NUM_10, 1);
                deepsleep_init_params.wakeup_time_sec = value;
                esp_sleep_enable_timer_wakeup(deepsleep_init_params.wakeup_time_sec * 1000000);
                ESP_LOGI("DEEPSLEEP","GOING TO SLEEP for %ds! GOODNIGHT!",value);
                esp_deep_sleep_start();
                break;
            }
            else
            {
                for (uint8_t i = 0; i < RF_MAX_ENUMS; i++)
                {
                    if(!strcmp(value_string, rf_module_trigger_token[i])){
                        if(i != RF_IDLE ) apply_trigger(i);
                        break;
                    }
                }
                break;
            }
        }
        break;
    }
    default:
    {
        break;
    }
    }
    return ret;
}

/*Function for RF*/
int device_0070_RF_module(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void* user_arg)
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

static int __prepare(void *arg)
{
    int ret = 0;
    ezlopi_rf_action_struct_t *rf_module_state = (ezlopi_rf_action_struct_t*)malloc(sizeof(ezlopi_rf_action_struct_t));
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg && prep_arg->cjson_device)
    {
        cJSON *cj_device = prep_arg->cjson_device;
        l_ezlopi_device_t *device = ezlopi_device_add_device(cj_device);
            if (device)
            {
                __setup_device_cloud_params(device, cj_device);
                rf_module_item = ezlopi_device_add_item_to_device(device, device_0070_RF_module);
                if (rf_module_item)
                {
                    rf_module_item->cloud_properties.show = true;
                    rf_module_item->cloud_properties.has_getter = true;
                    rf_module_item->cloud_properties.has_setter = true;
                    rf_module_item->cloud_properties.item_name = ezlopi_item_name_energy_saving_mode;
                    rf_module_item->cloud_properties.value_type = value_type_token;
                    rf_module_item->cloud_properties.scale = NULL;
                    rf_module_item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
                    /* initialize rf structure */
                    rf_module_state->current_trigger_value = "unknown";
                    rf_module_item->user_arg = rf_module_state;

                    rf_module_item->interface_type = EZLOPI_DEVICE_INTERFACE_SPI_MASTER;
                    CJSON_GET_VALUE_INT(cj_device, "miso", rf_module_item->interface.spi_master.miso);
                    CJSON_GET_VALUE_INT(cj_device, "mosi", rf_module_item->interface.spi_master.mosi);
                    CJSON_GET_VALUE_INT(cj_device, "sck", rf_module_item->interface.spi_master.sck);
                    CJSON_GET_VALUE_INT(cj_device, "cs", rf_module_item->interface.spi_master.cs);
                }
            }
    }

    return ret;
}

static int __init(l_ezlopi_item_t *item)
{
    int ret = 0;
    if (
        GPIO_IS_VALID_GPIO(item->interface.spi_master.mosi) &&
        GPIO_IS_VALID_GPIO(item->interface.spi_master.mosi) &&
        GPIO_IS_VALID_GPIO(item->interface.spi_master.sck)  &&
        GPIO_IS_VALID_GPIO(item->interface.spi_master.cs)
        )
    {
        ezlopi_spi_master_init(&item->interface.spi_master);
        trans_recv_init(RECIEVER);//TODO: remove spi init from transrecvinit()
        // trans_recv_init(TRANSMITTER);//TODO: remove spi init from transrecvinit()
        deepsleep_init_params.wakeup_time_sec = 10;
        ret = 1;
    }
    return ret;
}

static int __get_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;

    cJSON *param = (cJSON *)arg;
    ezlopi_rf_action_struct_t *rf_module_state = (ezlopi_rf_action_struct_t *)item->user_arg;
    if (param)
    {
        if (ezlopi_item_name_energy_saving_mode == item->cloud_properties.item_name)
        {
            //-------------------  POSSIBLE JSON ENUM CONTENTS ----------------------------------
            cJSON *json_array_enum = cJSON_CreateArray();
            if (NULL != json_array_enum)
            {
                for (uint8_t i = 0; i < RF_MAX_ENUMS; i++)
                {
                    cJSON *json_value = cJSON_CreateString(rf_module_trigger_token[i]);
                    if (NULL != json_value)
                    {
                        cJSON_AddItemToArray(json_array_enum, json_value);
                    }
                }
                cJSON_AddItemToObject(param, "enum", json_array_enum);
            }
            cJSON_AddStringToObject(param, "value", rf_module_state->current_trigger_value);

        }
    }

    return ret;
}

static int __set_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;

    cJSON *device_details = (cJSON *)arg;
    ezlopi_rf_action_struct_t *rf_module_state = (ezlopi_rf_action_struct_t *)item->user_arg;
    if (device_details)
    {
        if (ezlopi_item_name_energy_saving_mode == item->cloud_properties.item_name)
        {
            // CJSON_GET_VALUE_INT(device_details, "value", value);
            char *value = NULL;
            CJSON_GET_VALUE_STRING(device_details, "value", value);
            // ESP_LOGW("Rceived Value 1", "%s", value);
            // strcpy(rf_module_state->previous_trigger_value, rf_module_state->current_trigger_value);
            // ESP_LOGW("Rceived Value 1", "%s", rf_module_state->previous_trigger_value);
            strcpy(rf_module_state->current_trigger_value, value);
            ezlopi_device_value_updated_from_device_v3(rf_module_item);
            ESP_LOGW("Received Value", "%s", rf_module_state->current_trigger_value);

            /* loop through each token and apply which is received */
            for (uint8_t i = 0; i < RF_MAX_ENUMS; i++)
            {
                if(!strcmp(value, rf_module_trigger_token[i])){
                    if(i != RF_IDLE ) apply_trigger(i);
                    break;
                }
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

    l_ezlopi_device_settings_v3_t *device_setting = (l_ezlopi_device_settings_v3_t*)malloc(sizeof(l_ezlopi_device_settings_v3_t));
    device_setting->cloud_properties.setting_id = ezlopi_cloud_generate_item_id();
    device_setting->cloud_properties.device_id = device->cloud_properties.device_id;
    device_setting->func = device_0070_DeepSleep_settings;
    device_setting->next = NULL;

    device->settings = device_setting;
}