#ifndef DEVICE_0022_PWM_DIMMABLE_LAMP_H
#define DEVICE_0022_PWM_DIMMABLE_LAMP_H
#include "ezlopi_actions.h"
#include "ezlopi_devices.h"

typedef struct ezlopi_dimmable_bulb_state_struct{
    uint32_t previous_brightness_value;
    uint32_t current_brightness_value;
}ezlopi_dimmable_bulb_state_struct_t;

int device_0022_PWM_dimmable_lamp(e_ezlopi_actions_t action, s_ezlopi_device_properties_t *ezlo_device, void *arg, void *user_arg);

#endif // DEVICE_0022_PWM_DIMMABLE_LAMP_H

