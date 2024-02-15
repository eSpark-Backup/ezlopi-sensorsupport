#ifndef _DEVICE_0070_DeepSleep_
#define _DEVICE_0070_DeepSleep_

#include "ezlopi_core_actions.h"
#include "ezlopi_core_devices.h"

typedef struct ezlopi_deepsleep_action
{
    char* previous_trigger_value;
    char* current_trigger_value;
} ezlopi_deepsleep_action_struct_t;

typedef enum
{
    MIN_10,
    MIN_20,
    MIN_30,
    MIN_40,
    DEEPSLEEP_MAX_ENUMS

} e_deepsleep_trigger_states_t;

typedef struct deepsleep_parameters 
{
    struct timeval now;
    int wakeup_time_sec;
    int sleep_time_ms;
} deepsleep_parameters_t;

int device_0070_DeepSleep(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void* user_arg);

#endif // __DEVICE_0070_DEEPSLEEP