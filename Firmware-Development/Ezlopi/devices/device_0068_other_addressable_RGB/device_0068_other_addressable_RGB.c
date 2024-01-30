#include "cJSON.h"
#include "trace.h"

#include "esp_netif.h"

#include "ezlopi_wifi.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_cloud_device_types_str.h"
#include "ezlopi_cloud_category_str.h"
#include "ezlopi_cloud_subcategory_str.h"
#include "ezlopi_cloud_value_type_str.h"
#include "ezlopi_cloud_item_name_str.h"
#include "ezlopi_cloud_keywords.h"
#include "ezlopi_valueformatter.h"

#include "device_0068_other_addressable_RGB.h"

/* Argb led driver Specfic includes*/
#include "config.h"
#include "led.h"
#include "led_server.h"

/* Global Declarations */
static l_ezlopi_device_t *device = NULL;
static bool ws2812_led_strip_initialized = false;
struct LED_CONFIG* led_config;

/* Prototype */
static int __prepare(void *arg);
static void __prepare_device_properties(l_ezlopi_device_t *device, cJSON *cj_device);
static void __prepare_WS2812_LED_onoff_switch_item(l_ezlopi_item_t *item, cJSON *cj_device);
static int device_0068_other_addressable_RGB_settings(e_ezlopi_settings_action_t action, l_ezlopi_device_settings_v3_t *setting, void *arg, void *user_arg);

static int __init(l_ezlopi_item_t *item);
static int __get_cjson_value(l_ezlopi_item_t *item, void *arg);

int device_0068_other_addressable_RGB(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg)
{
    int ret = 0;
    
    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
        __prepare(arg);
        break;
    case EZLOPI_ACTION_INITIALIZE:
        __init(item);
        break;
    case EZLOPI_ACTION_HUB_GET_ITEM:
        __get_cjson_value(item, arg);
        break;
    default:
        break;
    }

    return ret;
}

/* Static Helper Functions */
static int __prepare(void *arg)
{
    int ret = 0;

    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    cJSON *cjson_device = prep_arg->cjson_device;

    if (prep_arg && prep_arg->cjson_device)
    {
        /*Check If there is device already*/
        l_ezlopi_device_t *curr_device = ezlopi_device_get_head();
        if(NULL == curr_device)
        {
            curr_device = ezlopi_device_add_device(cjson_device);
        }
        device = curr_device;

        /*if device, add items*/
        if (device)
        {
            /*add properties to the device*/
            __prepare_device_properties(device, prep_arg->cjson_device);

            /*select device item*/
            l_ezlopi_item_t *switch_item = device->items;

            switch_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            if(switch_item)
            {
                switch_item->cloud_properties.device_id = device->cloud_properties.device_id;
                switch_item->user_arg = switch_item;
            }

            /*now prepare all items which are added*/
            __prepare_WS2812_LED_onoff_switch_item(switch_item, prep_arg->cjson_device);
        }
        else
        {
            ezlopi_device_free_device(device);
        }
    }

    return ret;
}

static void __prepare_device_properties(l_ezlopi_device_t *device, cJSON *cj_device)
{
    char *device_name = NULL;
    CJSON_GET_VALUE_STRING(cj_device, "dev_name", device_name);
    ASSIGN_DEVICE_NAME_V2(device, device_name);

    device->cloud_properties.category = category_led_strip;
    device->cloud_properties.subcategory = subcategory_addressable_main;
    device->cloud_properties.device_type = dev_type_device;

    device->cloud_properties.device_id = ezlopi_cloud_generate_device_id();

    l_ezlopi_device_settings_v3_t *device_setting = (l_ezlopi_device_settings_v3_t*)malloc(sizeof(l_ezlopi_device_settings_v3_t));
    device_setting->cloud_properties.setting_id = ezlopi_cloud_generate_item_id();
    device_setting->cloud_properties.device_id = device->cloud_properties.device_id;
    device_setting->func = device_0068_other_addressable_RGB_settings;
    device_setting->next = NULL;

    device->settings = device_setting;
}

static void __prepare_WS2812_LED_onoff_switch_item(l_ezlopi_item_t *item, cJSON *cj_device)
{
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_switch,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT;
    item->interface.gpio.gpio_out.enable = true;
    CJSON_GET_VALUE_INT(cj_device, "gpio", item->interface.gpio.gpio_out.gpio_num);
    item->interface.gpio.gpio_out.interrupt = false;
    item->interface.gpio.gpio_out.invert = false;
    item->interface.gpio.gpio_out.mode = GPIO_MODE_OUTPUT;
    item->interface.gpio.gpio_out.pull = false;
    item->interface.gpio.gpio_out.value = false;
    item->interface.gpio.gpio_in.enable = false;
}

/* Setting Handling */
static int device_0068_other_addressable_RGB_settings(e_ezlopi_settings_action_t action, l_ezlopi_device_settings_v3_t *setting, void *arg, void *user_arg)
{
    int ret = 0;
    switch (action)
    {
    case EZLOPI_SETTINGS_ACTION_GET_SETTING:
    {
        cJSON* params = (cJSON*)arg;

        cJSON *label = cJSON_CreateObject();
        cJSON_AddItemToObject(params, "label", label);

        // Add properties to "label" object
        cJSON_AddStringToObject(label, "text", "IP Address");
        cJSON_AddStringToObject(label, "lang_tag", "addressable_led_ip_address_label");

        cJSON *desc = cJSON_CreateObject();
        cJSON_AddItemToObject(params, "description", desc);

        // Add properties to "label" object
        cJSON_AddStringToObject(desc, "text", "The IP address of this instance.");
        cJSON_AddStringToObject(desc, "lang_tag", "addressable_led_ip_address_label");

        cJSON_AddBoolToObject(params, "hasSetter", false);
        cJSON_AddStringToObject(params, "valueType", "string");
        
        char tmp_string[64];
        esp_netif_ip_info_t *ip_info = ezlopi_wifi_get_ip_infos();
        snprintf(tmp_string, sizeof(tmp_string), IPSTR, IP2STR(&ip_info->ip));

        cJSON_AddStringToObject(params, "value", tmp_string);
    
        
        break;
    }
    default:
    {
        break;
    }
    }
    return ret;
}

static int __init(l_ezlopi_item_t *item)
{
    int ret = 0;
    
    if(!ws2812_led_strip_initialized)
    {
        // vTaskDelay(5000/portTICK_PERIOD_MS);
        // server_init();
        start_webserver();
        vTaskDelay(1000/portTICK_PERIOD_MS);
        config_init();
        led_on(item->interface.gpio.gpio_out.gpio_num);
        config_update_channels();
        led_config = config_return_config_pointer();
        ws2812_led_strip_initialized = true;
    }

    return ret;
}

static int __get_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;

    cJSON *cjson_properties = (cJSON *)arg;
    if ((NULL != cjson_properties))
    {
        if (ezlopi_item_name_switch == item->cloud_properties.item_name)
        {
            item->interface.gpio.gpio_in.value = 1; //TODO:
            cJSON_AddBoolToObject(cjson_properties, ezlopi_value_str, item->interface.gpio.gpio_in.value);
            char *formatted_val = ezlopi_valueformatter_bool(item->interface.gpio.gpio_in.value ? true : false);
            cJSON_AddStringToObject(cjson_properties, ezlopi_valueFormatted_str, formatted_val);
        }
    }
    return ret;
}