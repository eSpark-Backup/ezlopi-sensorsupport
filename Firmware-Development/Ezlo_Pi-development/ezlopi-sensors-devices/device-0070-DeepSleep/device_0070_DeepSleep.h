#ifndef _DEVICE_0070_DeepSleep_
#define _DEVICE_0070_DeepSleep_

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices.h"

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

typedef struct deepsleep_parameters 
{
    struct timeval now;
    int wakeup_time_sec;
    int sleep_time_ms;
} deepsleep_parameters_t;

int device_0070_RF_module(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void* user_arg);

#endif // __DEVICE_0070_DEEPSLEEP