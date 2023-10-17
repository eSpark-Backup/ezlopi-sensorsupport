#pragma once

#include <stdbool.h>
#include "driver/gpio.h"

#define DUTY_CYCLE_50_PERCENT_VAL 16 // For 5 bit timer resolution

#define PCNT_H_LIM_VAL      32767U
#define PCNT_L_LIM_VAL     -10
#define PCNT_THRESH1_VAL    5
#define PCNT_THRESH0_VAL   -5
#define PCNT_INPUT_SIG_IO   4  // Pulse Input GPIO
#define PCNT_INPUT_CTRL_IO  -1  // Control GPIO HIGH=count up, LOW=count down

typedef enum a4988_microstep
{
    FULL_STEP=1,
    HALF_STEP=2,
    QUARTER_STEP=4,
    EIGHTH_STEP=8,
    SIXTEENTH_STEP=16,
} a4988_microstep_t;

typedef struct a4988_config
{
    gpio_num_t step_pin;
    gpio_num_t counter_pin;
    gpio_num_t direction_pin;
    gpio_num_t en_pin;
    gpio_num_t ms_pin[3];
    a4988_microstep_t microsteps;
    int32_t steps_left;
    uint16_t steps_per_rev;
    int16_t steps;
    bool micro_step_gpio_control;
	
} a4988_config_t;

typedef enum a4988_run_mode
{
    STEPS_MODE,
    ANGLE_MODE,
    ROTATION_MODE,
} a4988_run_mode_t;

typedef enum a4988_direction
{
    CLOCKWISE,
    COUNTER_CLOCKWISE
} a4988_direction_t;

typedef enum a4988_status
{
    A4988_NOT_INITIALIZED,
    A4988_INITIALIZED,
} a4988_status_t;

typedef struct motor_run_properties
{
    float rpm;
    uint32_t steps;
    float angle;
    float rotations;
    a4988_direction_t direction;
    a4988_run_mode_t mode;
} motor_run_properties_t;

/* A sample structure to pass events from the PCNT
 * interrupt handler to the main program.
 */
typedef struct {
    int unit;  // the PCNT unit that originated an interrupt
    uint32_t status; // information on the event type that caused the interrupt
} pcnt_evt_t;

esp_err_t a4988_setup(a4988_config_t* config);
void a4988_set_direction(a4988_direction_t direction);
void a4988_enable_driver();
void a4988_disable_driver();
void a4988_run_speed(float rpm);
bool a4988_run(motor_run_properties_t* motor_properties);
bool a4988_is_initialized();
void a4988_set_steps(uint32_t steps);
void a4988_set_microsteps(a4988_microstep_t microsteps);
void a4988_update_speed(float rpm);

// Example 
/*
a4988_config_t config = {
        .en_pin = GPIO_NUM_21, 
        .step_pin = GPIO_NUM_18, 
        .counter_pin = GPIO_NUM_4,
        .direction_pin = GPIO_NUM_19,
        .ms_pin[0] = GPIO_NUM_5,
        .ms_pin[1] = GPIO_NUM_22,
        .ms_pin[2] = GPIO_NUM_23, 
        .steps_per_rev = 200,
        .microsteps = 16,
        .micro_step_gpio_control = true,
    };
    
    a4988_setup(&config);
    a4988_enable_driver();
    motor_run_properties_t motor_properties = {.rpm = 60, .angle = 720, .direction = CLOCKWISE, .mode = ANGLE_MODE};
    a4988_run(&motor_properties);
*/