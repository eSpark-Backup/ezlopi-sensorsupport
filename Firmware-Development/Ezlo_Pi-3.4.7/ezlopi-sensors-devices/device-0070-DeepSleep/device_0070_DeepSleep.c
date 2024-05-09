#include "esp_log.h"
#include <sys/time.h>
#include "esp_sleep.h"
#include "trans_recv.h"
#include <stdio.h>
#include <inttypes.h>
#include "driver/timer.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "ezlopi_cloud_items.h"
#include "ezlopi_cloud_settings.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_core_cjson_macros.h"
#include "ezlopi_core_device_value_updated.h"

#include "device_0070_DeepSleep.h"

esp_err_t err;
nvs_handle_t my_handle;
deepsleep_parameters_t deepsleep_init_params;

timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = TIMER_AUTORELOAD_EN
};

/*RF Control*/
static bool rf_module_initialized = false;
const char *rf_module_trigger_token[] =
    {
        "idle",
        "sleep",
        "shutdown",
        "unknown"
    };

static Sync_Words_t      *Sync_Words       = NULL;
static l_ezlopi_item_t   *rf_module_item   = NULL;
static cc1101_gpios_t    *cc1101_gpios_ptr = NULL;
static DelayTaskParams_t *DelayTaskParams  = NULL;

static int __prepare(void *arg);
static void delay_task(void *parameter);
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
            cJSON *value_root = cJSON_GetObjectItem(params,"value");
            // DelayTaskParams->mode = cJSON_GetObjectItem(value_root,"mode")->valuestring;
            strcpy(DelayTaskParams->mode, cJSON_GetObjectItem(value_root,"mode")->valuestring);
            DelayTaskParams->sleep_time = cJSON_GetObjectItem(value_root,"sleep_s")->valueint;
            DelayTaskParams->wakeup_time = cJSON_GetObjectItem(value_root,"wakeup_s")->valueint;
            DelayTaskParams->delay_time = cJSON_GetObjectItem(value_root,"delay_s")->valueint;
            printf("Mode:%s\n",DelayTaskParams->mode);
            printf("Sleep_time=%d\n",DelayTaskParams->sleep_time);
            printf("Wakeup_time=%d\n",DelayTaskParams->wakeup_time);
            printf("Delay_time=%d\n",DelayTaskParams->delay_time);
            // ESP_LOGI("DEEPSLEEP", "Mode:%s",DelayTaskParams->mode);
            // ESP_LOGI("DEEPSLEEP", "sleep_time=%d",DelayTaskParams->sleep_time);
            // ESP_LOGI("DEEPSLEEP", "wakeup_time=%d",DelayTaskParams->wakeup_time);
            // ESP_LOGI("DEEPSLEEP", "delay_time=%d",DelayTaskParams->delay_time);
            xTaskCreate(delay_task,"delay_task",4096,(void *)DelayTaskParams,2,NULL);
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
    Sync_Words = (Sync_Words_t *)malloc(sizeof(Sync_Words_t));
    cc1101_gpios_ptr = (cc1101_gpios_t *)malloc(sizeof(cc1101_gpios_t));
    DelayTaskParams = (DelayTaskParams_t *)malloc(sizeof(DelayTaskParams_t));
    DelayTaskParams->mode = (char *)malloc(sizeof(char)*10);  // Allocate memory for mode
    ezlopi_rf_action_struct_t *rf_module_state = (ezlopi_rf_action_struct_t*)malloc(sizeof(ezlopi_rf_action_struct_t));
    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg && prep_arg->cjson_device)
    {
        cJSON *cj_device = prep_arg->cjson_device;
        l_ezlopi_device_t *device = ezlopi_device_add_device(cj_device,NULL);
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
                CJSON_GET_VALUE_INT(cj_device, ezlopi_gpio_miso_str, rf_module_item->interface.spi_master.miso);
                CJSON_GET_VALUE_INT(cj_device, ezlopi_gpio_mosi_str, rf_module_item->interface.spi_master.mosi);
                CJSON_GET_VALUE_INT(cj_device, ezlopi_gpio_sck_str, rf_module_item->interface.spi_master.sck);
                CJSON_GET_VALUE_INT(cj_device, ezlopi_gpio_ss_str, rf_module_item->interface.spi_master.cs);
                CJSON_GET_VALUE_INT(cj_device, ezlopi_gpio_gdo0_str, cc1101_gpios_ptr->gdo0);
                CJSON_GET_VALUE_INT(cj_device, ezlopi_gpio_gdo2_str, cc1101_gpios_ptr->gdo2);
                CJSON_GET_VALUE_INT(cj_device, ezlopi_gpio_shutdown_str, cc1101_gpios_ptr->shutdown);
            }
        }
    }

    return ret;
}

static int __init(l_ezlopi_item_t *item)
{
    int ret = 0;
    uint8_t mac_addr[6];
    if (
        GPIO_IS_VALID_GPIO(item->interface.spi_master.miso) &&
        GPIO_IS_VALID_GPIO(item->interface.spi_master.mosi) &&
        GPIO_IS_VALID_GPIO(item->interface.spi_master.sck)  &&
        GPIO_IS_VALID_GPIO(item->interface.spi_master.cs)
        )
    {
        // Sync_Words->ON_SYNC_WORD         = 0xFA50A6B3;
        // Sync_Words->WAKE_UP_SYNC_WORD    = 0xFA50A6B4;
        // Sync_Words->SHUTDOWN_SYNC_WORD   = 0xFA50A6B5;
        // printf("Merged Sync Word:%x\n",Sync_Words->ON_SYNC_WORD);
        // printf("Merged Sync Word:%x\n",Sync_Words->WAKE_UP_SYNC_WORD);
        // printf("Merged Sync Word:%x\n",Sync_Words->SHUTDOWN_SYNC_WORD);
        esp_read_mac(mac_addr, ESP_MAC_WIFI_STA);
        printf("MAC Address ");
        for(int i=0;i<=5;i++)
        {
            printf(":%x", mac_addr[i]);
        }
        printf("\n");
        Sync_Words->unique_id = (((mac_addr[2]<<8)|mac_addr[3])<<8|mac_addr[4])<<8|mac_addr[5];
        Sync_Words->ON_SYNC_WORD         = Sync_Words->unique_id + ON_SYNC_WORD_BYTE;
        Sync_Words->WAKE_UP_SYNC_WORD    = Sync_Words->unique_id + WAKE_UP_SYNC_WORD_BYTE;
        Sync_Words->SHUTDOWN_SYNC_WORD   = Sync_Words->unique_id + SHUTDOWN_SYNC_WORD_BYTE;
        printf("Merged Sync Word:%x\n",Sync_Words->ON_SYNC_WORD);
        printf("Merged Sync Word:%x\n",Sync_Words->WAKE_UP_SYNC_WORD);
        printf("Merged Sync Word:%x\n",Sync_Words->SHUTDOWN_SYNC_WORD);
        err = nvs_flash_init();
        err = nvs_open("storage", NVS_READWRITE, &my_handle);
        err = nvs_get_i32(my_handle, "wakeup_time", &DelayTaskParams->wakeup_time);
        err = nvs_get_i32(my_handle, "sleep_time", &DelayTaskParams->sleep_time);
        // Read
        ESP_LOGI("WAKEUP_NVS","Reading wakeup_time from NVS ... ");
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                printf("Wakeup_time = %d\n", DelayTaskParams->wakeup_time);
                printf("Sleep_time = %d\n", DelayTaskParams->sleep_time);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
        nvs_close(my_handle);
        if(DelayTaskParams->sleep_time > 0 && DelayTaskParams->wakeup_time >90)
        {
            timer_init(TIMER_GROUP_0, TIMER_1, &config);
            timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
            timer_set_alarm_value(TIMER_GROUP_0, TIMER_1, (DelayTaskParams->wakeup_time*(TIMER_BASE_CLK / TIMER_DIVIDER)));
            timer_enable_intr(TIMER_GROUP_0, TIMER_1);
            timer_isr_callback_add(TIMER_GROUP_0, TIMER_1, timer_group_isr_callback, DelayTaskParams->sleep_time, 0);
            timer_start(TIMER_GROUP_0, TIMER_1);
        }
        trans_recv_init(RECIEVER, &item->interface.spi_master, cc1101_gpios_ptr, Sync_Words);
        // trans_recv_init(TRANSMITTER, &item->interface.spi_master, cc1101_gpios_ptr, Sync_Words);
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
            char *value = NULL;
            CJSON_GET_VALUE_STRING(device_details, "value", value);
            // ESP_LOGW("Rceived Value 1", "%s", value);
            // strcpy(rf_module_state->previous_trigger_value, rf_module_state->current_trigger_value);
            // ESP_LOGW("Rceived Value 1", "%s", rf_module_state->previous_trigger_value);
            strcpy(rf_module_state->current_trigger_value, value);
            ezlopi_device_value_updated_from_device_v3(rf_module_item);
            ESP_LOGI("Received Value", "%s", rf_module_state->current_trigger_value);

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

static void delay_task(void *parameter)
{
    DelayTaskParams_t *params = (DelayTaskParams_t *)parameter;
    char *mode=params->mode;
    // ESP_LOGI("DEEPSLEEP", "Mode:%s",params->mode);
    // ESP_LOGI("DEEPSLEEP", "Mode:%d",params->sleep_time);
    vTaskDelay((params->delay_time*1000)/portTICK_PERIOD_MS);
    if(strcmp(params->mode, "sleep") == 0)
    {
        wakeup_func();
        if(params->sleep_time > 0)
        {
            printf("Timer value from the cloud: %ds\nGOING TO SLEEP for %ds! GOODNIGHT!\n",params->sleep_time,params->sleep_time);
            // ESP_LOGI("DEEPSLEEP","Timer value from the cloud: %ds",params->sleep_time);
            CC1101_setSyncWord(Sync_Words->WAKE_UP_SYNC_WORD, 145);
            esp_sleep_enable_ext0_wakeup(cc1101_gpios_ptr->gdo0, 1);
            // ESP_LOGI("DEEPSLEEP","GOING TO SLEEP for %ds! GOODNIGHT!",params->sleep_time);
            esp_deep_sleep(params->sleep_time*1000000);
        }
        else if(params->sleep_time == 0)
        {
            CC1101_setSyncWord(Sync_Words->WAKE_UP_SYNC_WORD, 145);
            esp_sleep_enable_ext0_wakeup(cc1101_gpios_ptr->gdo0, 1);
            printf("GOODNIGHT!\n");
            // ESP_LOGI("DEEPSLEEP", "SLEEP FOR INFINITE PERIOD!");
            esp_deep_sleep_start();
        }
    }
    else
    {
        CC1101_setSyncWord(Sync_Words->SHUTDOWN_SYNC_WORD, 145);
        gpio_set_direction(cc1101_gpios_ptr->shutdown, GPIO_MODE_OUTPUT);
        gpio_set_level(cc1101_gpios_ptr->shutdown, HIGH);
        printf("ESP SHUTTING DOWN!\n");
        // ESP_LOGI("DEEPSLEEP", "ESP SHUTTING DOWN");
    }
    vTaskDelete(NULL);
}

void wakeup_func()
{
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    err = nvs_get_i32(my_handle, "wakeup_time", &DelayTaskParams->wakeup_time_fresh);
    if (err != ESP_OK) 
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else 
    {
        printf("Done\n");
        // Write
        printf("Updating restart counter in NVS ... ");
        err = nvs_set_i32(my_handle, "sleep_time", DelayTaskParams->sleep_time);
        if(DelayTaskParams->wakeup_time_fresh != DelayTaskParams->wakeup_time)
        {
            err = nvs_set_i32(my_handle, "wakeup_time", DelayTaskParams->wakeup_time);
        }
        else
        {
            err = nvs_set_i32(my_handle, "wakeup_time", DelayTaskParams->wakeup_time_fresh);
        }
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle);
    }
}

static bool IRAM_ATTR timer_group_isr_callback(void * args)
{
    esp_deep_sleep(DelayTaskParams->sleep_time*1000000);
    return 0;
}