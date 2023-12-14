#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "sdkconfig.h"
#include "dictionary.h"
#include "trace.h"

// #include "web_provisioning.h"

#include "cJSON.h"
#include "ezlopi_devices.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_cloud_methods_str.h"
#include "ezlopi_devices_list.h"
// #include "ezlopi_cloud_value_type_str.h"
#include "web_provisioning.h"

void dictionary_clear_v3(cJSON *cj_request, cJSON *cj_response)
{
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_id_str, cJSON_GetObjectItem(cj_request, ezlopi_id_str));
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_key_method_str, cJSON_GetObjectItem(cj_request, ezlopi_key_method_str));
    cJSON_AddObjectToObject(cj_response, ezlopi_result_str);

    cJSON *cj_params = cJSON_GetObjectItem(cj_request, ezlopi_params_str);
    if (cj_params)
    {
        char *item_id_str = 0;
        CJSON_GET_VALUE_STRING(cj_params, ezlopi__id_str, item_id_str);
        int item_id = strtol(item_id_str, NULL, 16);
        TRACE_I("item_id: %X", item_id);

        l_ezlopi_device_t *curr_device = ezlopi_device_get_head();
        uint32_t found_item = 0;
        while (curr_device)
        {
            l_ezlopi_item_t *curr_item = curr_device->items;
            while (curr_item)
            {
                if (item_id == curr_item->cloud_properties.item_id)
                {
                    curr_item->func(EZLOPI_ACTION_DICTIONARY_CLEAR, curr_item, cj_params, curr_item->user_arg);
                    found_item = 1;
                    break;
                }
                curr_item = curr_item->next;
            }
            if (found_item)
            {
                break;
            }
            curr_device = curr_device->next;
        }
    }
}

void dictionary_remove_value_v3(cJSON *cj_request, cJSON *cj_response)
{
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_id_str, cJSON_GetObjectItem(cj_request, ezlopi_id_str));
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_key_method_str, cJSON_GetObjectItem(cj_request, ezlopi_key_method_str));
    cJSON_AddObjectToObject(cj_response, ezlopi_result_str);

    cJSON *cj_params = cJSON_GetObjectItem(cj_request, ezlopi_params_str);
    if (cj_params)
    {
        char *item_id_str = 0;
        CJSON_GET_VALUE_STRING(cj_params, ezlopi__id_str, item_id_str);
        int item_id = strtol(item_id_str, NULL, 16);
        TRACE_I("item_id: %X", item_id);

        l_ezlopi_device_t *curr_device = ezlopi_device_get_head();
        uint32_t found_item = 0;
        while (curr_device)
        {
            l_ezlopi_item_t *curr_item = curr_device->items;
            while (curr_item)
            {
                if (item_id == curr_item->cloud_properties.item_id)
                {
                    curr_item->func(EZLOPI_ACTION_REMOVE_VALUE, curr_item, cj_params, curr_item->user_arg);
                    found_item = 1;
                    break;
                }
                curr_item = curr_item->next;
            }
            if (found_item)
            {
                break;
            }
            curr_device = curr_device->next;
        }
    }
}

void dictionary_add_value_v3(cJSON *cj_request, cJSON *cj_response)
{
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_id_str, cJSON_GetObjectItem(cj_request, ezlopi_id_str));
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_key_method_str, cJSON_GetObjectItem(cj_request, ezlopi_key_method_str));
    cJSON_AddObjectToObject(cj_response, ezlopi_result_str);

    cJSON *cj_params = cJSON_GetObjectItem(cj_request, ezlopi_params_str);
    if (cj_params)
    {
        char *item_id_str = 0;
        CJSON_GET_VALUE_STRING(cj_params, ezlopi__id_str, item_id_str);
        int item_id = strtol(item_id_str, NULL, 16);
        TRACE_I("item_id: %X", item_id);

        l_ezlopi_device_t *curr_device = ezlopi_device_get_head();
        uint32_t found_item = 0;
        while (curr_device)
        {
            l_ezlopi_item_t *curr_item = curr_device->items;
            while (curr_item)
            {
                if (item_id == curr_item->cloud_properties.item_id)
                {
                    curr_item->func(EZLOPI_ACTION_ADD_VALUE, curr_item, cj_params, curr_item->user_arg);
                    found_item = 1;
                    break;
                }
                curr_item = curr_item->next;
            }
            if (found_item)
            {
                break;
            }
            curr_device = curr_device->next;
        }
    }
}

void dictionary_set_value_v3(cJSON *cj_request, cJSON *cj_response)
{
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_id_str, cJSON_GetObjectItem(cj_request, ezlopi_id_str));
    cJSON_AddItemReferenceToObject(cj_response, ezlopi_key_method_str, cJSON_GetObjectItem(cj_request, ezlopi_key_method_str));
    cJSON_AddObjectToObject(cj_response, ezlopi_result_str);

    cJSON *cj_params = cJSON_GetObjectItem(cj_request, ezlopi_params_str);
    if (cj_params)
    {
        char *item_id_str = 0;
        CJSON_GET_VALUE_STRING(cj_params, ezlopi__id_str, item_id_str);
        int item_id = strtol(item_id_str, NULL, 16);
        TRACE_I("item_id: %X", item_id);

        l_ezlopi_device_t *curr_device = ezlopi_device_get_head();
        uint32_t found_item = 0;
        while (curr_device)
        {
            l_ezlopi_item_t *curr_item = curr_device->items;
            while (curr_item)
            {
                if (item_id == curr_item->cloud_properties.item_id)
                {
                    curr_item->func(EZLOPI_ACTION_SET_VALUE, curr_item, cj_params, curr_item->user_arg);
                    found_item = 1;
                    break;
                }
                curr_item = curr_item->next;
            }
            if (found_item)
            {
                break;
            }
            curr_device = curr_device->next;
        }
    }
}

void dictionary_update_v3(cJSON *cj_request, cJSON *cj_response)
{
    cJSON *cjson_params = cJSON_GetObjectItem(cj_request, ezlopi_params_str);
    if (cjson_params)
    {
        char *item_id_str = NULL;
        CJSON_GET_VALUE_STRING(cjson_params, ezlopi__id_str, item_id_str);
        int item_id = strtol(item_id_str, NULL, 16);

        l_ezlopi_device_t *curr_device = ezlopi_device_get_head();
        while (curr_device)
        {
            l_ezlopi_item_t *curr_item = curr_device->items;
            while (curr_item)
            {
                if (item_id == curr_item->cloud_properties.item_id)
                {
                    cJSON_AddStringToObject(cj_response, ezlopi_id_str, ezlopi_ui_broadcast_str);
                    cJSON_AddStringToObject(cj_response, ezlopi_msg_subclass_str, method_hub_item_dictionary_updated);
                    cJSON *cj_result = cJSON_AddObjectToObject(cj_response, ezlopi_result_str);
                    if (cj_result)
                    {
                        char tmp_string[64];
                        cJSON_AddStringToObject(cj_result, "_id", item_id_str);
                        snprintf(tmp_string, sizeof(tmp_string), "%08x", curr_device->cloud_properties.device_id);
                        cJSON_AddStringToObject(cj_result, "deviceId", tmp_string);
                        cJSON_AddStringToObject(cj_result, "deviceName", curr_device->cloud_properties.device_name);
                        cJSON_AddStringToObject(cj_result, "deviceCategory", curr_device->cloud_properties.category);
                        cJSON_AddStringToObject(cj_result, "deviceSubcategory", curr_device->cloud_properties.subcategory);
                        cJSON_AddStringToObject(cj_result, "roomName", "");
                        cJSON_AddFalseToObject(cj_result, "serviceNotification");
                        cJSON_AddTrueToObject(cj_result, "userNotification");
                        cJSON_AddNullToObject(cj_result, "notifications");
                        cJSON_AddStringToObject(cj_result, "name", curr_item->cloud_properties.item_name);
                        cJSON_AddStringToObject(cj_result, "operation", "todoOperation");
                        if (curr_item->cloud_properties.scale)
                        {
                            cJSON_AddStringToObject(cj_result, "scale", curr_item->cloud_properties.scale);
                        }
                        curr_item->func(EZLOPI_ACTION_GET_EZLOPI_VALUE, curr_item, cj_result, curr_item->user_arg);
                        cJSON_AddStringToObject(cj_result, "valueType", curr_item->cloud_properties.value_type);
                    }

                    break;
                }

                curr_item = curr_item->next;
            }

            curr_device = curr_device->next;
        }
    }
}

cJSON *ezlopi_cloud_dictionary_updated_from_devices_v3(l_ezlopi_device_t *device, l_ezlopi_item_t *item)
{
    cJSON *cjson_response = cJSON_CreateObject();
    if (cjson_response)
    {
        if (NULL != item)
        {
            cJSON_AddStringToObject(cjson_response, ezlopi_msg_subclass_str, method_hub_item_dictionary_value_set);
            cJSON_AddNumberToObject(cjson_response, ezlopi_msg_id_str, web_provisioning_get_message_count());
            cJSON_AddStringToObject(cjson_response, ezlopi_id_str, ezlopi_ui_broadcast_str);

            cJSON *cj_result = cJSON_AddObjectToObject(cjson_response, ezlopi_result_str);
            if (cj_result)
            {
                char tmp_string[64];
                snprintf(tmp_string, sizeof(tmp_string), "%08x", item->cloud_properties.item_id);
                cJSON_AddStringToObject(cj_result, "_id", tmp_string);
                snprintf(tmp_string, sizeof(tmp_string), "%08x", device->cloud_properties.device_id);
                cJSON_AddStringToObject(cj_result, "deviceId", tmp_string);
                cJSON_AddStringToObject(cj_result, "deviceName", device->cloud_properties.device_name);
                cJSON_AddStringToObject(cj_result, "deviceCategory", device->cloud_properties.category);
                cJSON_AddStringToObject(cj_result, "deviceSubcategory", device->cloud_properties.subcategory);
                cJSON_AddStringToObject(cj_result, "roomName", ""); // roomName -> logic needs to be understood first
                cJSON_AddFalseToObject(cj_result, "serviceNotification");
                cJSON_AddFalseToObject(cj_result, "userNotification");
                cJSON_AddNullToObject(cj_result, "notifications");
                cJSON_AddFalseToObject(cj_result, "syncNotification");
                cJSON_AddStringToObject(cj_result, "name", item->cloud_properties.item_name);
                if (item->cloud_properties.scale)
                {
                    cJSON_AddStringToObject(cj_result, "scale", item->cloud_properties.scale);
                }
                item->func(EZLOPI_ACTION_GET_EZLOPI_VALUE, item, cj_result, item->user_arg);
                // registered_device->device->func(EZLOPI_ACTION_GET_EZLOPI_VALUE, registered_device->properties, cj_result, registered_device->user_arg);
                cJSON_AddStringToObject(cj_result, "valueType", item->cloud_properties.value_type);
            }
        }
        else
        {
            cJSON_Delete(cjson_response);
            cjson_response = NULL;
        }
    }

    return cjson_response;
}
