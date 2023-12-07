#ifndef __HUB_DICTIONARY_H__
#define __HUB_DICTIONARY_H__

#include "string.h"

#include "cJSON.h"
// #include "ezlopi_devices.h"
#include "ezlopi_devices_list.h"

#ifdef __cplusplus
extern "C"
{
#endif
    void dictionary_set_value_v3(cJSON *cj_request, cJSON *cj_response);
    void dictionary_update_v3(cJSON *cj_request, cJSON *cj_response);
    void dictionary_add_value_v3(cJSON *cj_request, cJSON *cj_response);
    void dictionary_remove_value_v3(cJSON *cj_request, cJSON *cj_response);
    void dictionary_clear_v3(cJSON *cj_request, cJSON *cj_response);

    cJSON *ezlopi_cloud_dictionary_updated_from_devices_v3(l_ezlopi_device_t *device, l_ezlopi_item_t *item);

#ifdef __cplusplus
}
#endif

#endif // __HUB_DICTIONARY_H__