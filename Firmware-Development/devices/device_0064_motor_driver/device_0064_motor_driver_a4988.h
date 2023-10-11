#ifndef _MOTOR_DRIVER_A4988_H_
#define _MOTOR_DRIVER_A4988_H_


#include "ezlopi_pwm.h"
#include "ezlopi_actions.h"
#include "ezlopi_devices.h"

#define MG996R_SERVO_FREQUENCY 50
#define MG996R_SERVO_DUTY_MAX 2200
#define MG996R_SERVO_DUTY_MIN 500

int device_0064_motor_driver_a4988(e_ezlopi_actions_t action, s_ezlopi_device_properties_t *ezlo_device, void *arg, void *user_arg);




#endif //_MOTOR_DRIVER_A4988_H_