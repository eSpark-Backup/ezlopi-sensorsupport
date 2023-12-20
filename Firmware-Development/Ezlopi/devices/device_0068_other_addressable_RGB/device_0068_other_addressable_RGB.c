
#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"

#include "cJSON.h"
#include "trace.h"

/* EzloPi's includes*/
#include "ezlopi_cloud.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_devices.h"
#include "ezlopi_device_value_updated.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_valueformatter.h"

#include "device_0068_other_addressable_RGB.h"

/* Heap information*/
#include "esp_heap_caps.h"
#include "esp_system.h"

/* Argb led driver Specfic includes*/
#include "config.h"
#include "led.h"

/* Device specific includes*/
#include "set_cjson_value.h"
#include "item_prepare.h"

static bool ws2812_led_strip_initialized = false;
struct LED_CONFIG* led_config;

static l_ezlopi_device_t *device = NULL;

static int __prepare(void *arg);
static int __init(l_ezlopi_item_t *item);
static int __set_cjson_value(l_ezlopi_item_t *item, void *arg);
static int __get_cjson_value(l_ezlopi_item_t *item, void *arg);
static int __add_cjson_value(l_ezlopi_item_t *item, void *arg);
static int __remove_cjson_value(l_ezlopi_item_t *item, void *arg);


int device_0068_other_addressable_RGB(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg)
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
        case EZLOPI_ACTION_ADD_VALUE:
        {
            ret = __add_cjson_value(item, arg);
            break;
        }
        case EZLOPI_ACTION_REMOVE_VALUE:
        {
            ret = __remove_cjson_value(item, arg);
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

    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg && prep_arg->cjson_device)
    {
        /*Check If there is device already*/
        l_ezlopi_device_t *curr_device = ezlopi_device_get_head();
        if(NULL == curr_device)
        {
            curr_device = ezlopi_device_add_device();
        }
        device = curr_device;

        /*if device, add items*/
        if (device)
        {
            /*add properties to the device*/
            __prepare_device_properties(device, prep_arg->cjson_device);

            /*select device item*/
            l_ezlopi_item_t *current_item = device->items;

            /*add unique items to parent device*/
            // for(int i = 0; i < sizeof(unique_parent_device_item_to_prepare) / sizeof(unique_parent_device_item_to_prepare[0]); i++)
            for(int i = 0; i < size_of_unique_parent(); i++)
            {
                current_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
                add_item_to_device_struct(current_item, return_prep_fcn_ptr(unique_parent_device_item_to_prepare[i]));
                ESP_LOGW("", "Added Unique Item");
                current_item = current_item->next;
            }

            /*add common items to parent device*/
            for(int i = 0; i < size_of_common_child(); i++)
            {
                current_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
                add_item_to_device_struct(current_item, return_prep_fcn_ptr(common_child_device_item_to_prepare[i]));
                ESP_LOGW("", "Added Common Item");
                current_item = current_item->next;
            }

            /*now prepare all items which are added*/
            prepare_all_items(prep_arg->cjson_device);
        }
        else
        {
            ezlopi_device_free_device(device);
        }
    }

    return ret;
}

static int __init(l_ezlopi_item_t *item)
{
    int ret = 0;
    
    if(!ws2812_led_strip_initialized)
    {
        config_init();
        led_on();
        config_update_channels();
        led_config = config_return_config_pointer();
        ws2812_led_strip_initialized = true;
    }

    return ret;
}

static int __set_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;
    if ((NULL != arg))
    {
        vTaskDelay(1000/portTICK_PERIOD_MS);
        processItem(item->cloud_properties.item_name, arg);
    }
    return ret;
}

static int __get_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;
    if (NULL != arg)
    {
        process_item_get(item->cloud_properties.item_name, arg);
    }
    return ret;
}

static int __add_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;

    char *segment_name = NULL;
    int segment_start = 0, segment_stop = 0;
    cJSON *cjson_params = (cJSON *)arg;

    if(NULL !=  cjson_params)
    {
        if (ezlopi_item_name_led_segments == item->cloud_properties.item_name)
        {
            cJSON* element = cJSON_GetObjectItem(cjson_params, "element");
            cJSON* value = cJSON_GetObjectItem(element, "value");
            CJSON_GET_VALUE_STRING(value, "name", segment_name);
            CJSON_GET_VALUE_INT(value, "start", segment_start);
            CJSON_GET_VALUE_INT(value, "end", segment_stop);

            config_set_segment_start_stop(config_get_channels(), segment_start, segment_stop);
            led_config->channel[config_get_channels()-1]->segment_name = strdup(segment_name);
        }

        /*==========================ADD NEW DEVICE FOR SEGMENT============================*/

        /*rest the item count every time when adding new device*/
        itemCount = 0;

        //iterate to last device and malloc next device
        l_ezlopi_device_t *curr_device = ezlopi_device_get_head();

        while (curr_device)
        {
            curr_device = curr_device->next;
        }

        /*malloc new device*/
        curr_device = ezlopi_device_add_device();
        l_ezlopi_device_t *new_device = curr_device;

        if(new_device)
        {
            /*Prepare Device*/
            cJSON* cj_device = cJSON_CreateObject();
            cJSON_AddStringToObject(cj_device, "dev_name", segment_name);

            /*prepare new device*/
            __prepare_device_properties(new_device, cj_device);

            // new_device->items =  ezlopi_device_add_item_to_device(new_device, device_0068_other_addressable_RGB);
            l_ezlopi_item_t *current_item = new_device->items;

            /*add items to device*/
            for(int i = 0; i < size_of_common_child(); i++)
            {
                current_item = ezlopi_device_add_item_to_device(new_device, device_0068_other_addressable_RGB);
                add_item_to_device_struct(current_item, return_prep_fcn_ptr(common_child_device_item_to_prepare[i]));
                current_item = current_item->next;
            }

            /*now prepare all items which are added*/
            prepare_all_items(cj_device);

        }
    }

    return ret;
}

static int __remove_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;
    char* key = 0;
    cJSON *cjson_params = (cJSON *)arg;
    if(NULL !=  cjson_params)
    {
        if (ezlopi_item_name_led_segments == item->cloud_properties.item_name)
        {
            CJSON_GET_VALUE_STRING(cjson_params, "key", key);

            l_ezlopi_device_t *current_device = ezlopi_device_get_head();

            for(int i = 0; i < config_get_channels(); i++)
            {
                if(i == atoi(key))
                {
                    ezlopi_device_free_device(current_device);
                    config_delete_segment(atoi(key));
                    break;
                }
                current_device = current_device->next;
            } 
        }
    }

    return ret;
}