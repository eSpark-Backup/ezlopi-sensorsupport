#ifndef _DEVICE_0067_RF_MODULE_
#define _DEVICE_0067_RF_MODULE_

#include "ezlopi_actions.h"
#include "ezlopi_devices.h"

typedef struct ezlopi_rf_action
{
    char* previous_trigger_value;
    char* current_trigger_value;
} ezlopi_rf_action_struct_t;

typedef enum
{
    RF_IDLE,
    RF_SLEEP,
    RF_TURNOFF,
    RF_MAX_ENUMS

} e_rf_module_trigger_states_t;

int device_0067_RF_module(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void* user_arg);

#endif // __DEVICE_0067_RF_MODULE__