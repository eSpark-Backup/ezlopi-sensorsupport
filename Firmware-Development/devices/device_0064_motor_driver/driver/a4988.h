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

typedef struct a4988_config
{
    gpio_num_t step_pin;
    gpio_num_t counter_gpio;
    gpio_num_t direction_pin;
    gpio_num_t en_pin;
    gpio_num_t ms1_pin;
    gpio_num_t ms2_pin;
    gpio_num_t ms3_pin;
    int32_t steps_left;
    uint16_t steps_per_rev;
    int16_t steps;
    bool micro_step_mode;
	
} a4988_config_t;

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
    a4988_direction_t direction;
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
void a4988_set_speed(float rpm);
bool a4988_run(motor_run_properties_t* motor_properties);
bool a4988_is_initialized();
void a4988_set_steps(uint32_t steps);

// Example 
/*
a4988_config_t config = {
    .en_pin = GPIO_NUM_21, 
    .step_pin = GPIO_NUM_18, 
    .direction_pin = GPIO_NUM_19, 
    .steps_per_rev = 200,
};

a4988_setup(&config);
a4988_enable_driver();
a4988_run(60, CLOCKWISE);
vTaskDelay(30 *1000 /portTICK_RATE_MS);
a4988_set_speed(0);
*/