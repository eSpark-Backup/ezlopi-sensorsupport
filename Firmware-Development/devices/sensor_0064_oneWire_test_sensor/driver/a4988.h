#pragma once

#include <stdbool.h>
#include "driver/gpio.h"

#define DUTY_CYCLE_50PERCENT_VALUE 16 // For 5_BIT_TIMER (2^5 / 2)

typedef struct a4988_config
{
    gpio_num_t step_pin;
    gpio_num_t direction_pin;
    gpio_num_t en_pin;
    gpio_num_t ms1_pin;
    gpio_num_t ms2_pin;
    gpio_num_t ms3_pin;
    uint16_t steps_per_rev;
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

esp_err_t a4988_setup(a4988_config_t* config);
void a4988_set_direction(a4988_direction_t direction);
void a4988_enable_driver();
void a4988_disable_driver();
void a4988_set_speed(float rpm);
void a4988_run(float rpm, a4988_direction_t direction);
bool a4988_is_initialized();

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