#include "a4988.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include <string.h>

static const char* TAG = "a4988 driver";

ledc_timer_config_t* ledc_timer = NULL;
ledc_channel_config_t* ledc_channel = NULL; 
a4988_config_t* a4988_driver_config = NULL;
a4988_status_t a4988_driver_status = A4988_NOT_INITIALIZED;

esp_err_t a4988_setup(a4988_config_t* config)
{
    if (A4988_INITIALIZED == a4988_driver_status)
    {
        ESP_LOGE(TAG, "a4988 driver already initialized");
        return ESP_FAIL;
    }

    esp_err_t ret = ESP_OK;
    
    a4988_driver_config = (a4988_config_t*)malloc(sizeof(a4988_config_t));
    ESP_GOTO_ON_FALSE(a4988_driver_config, ESP_ERR_NO_MEM, a4988_out, TAG, "no mem for a4988_config_t");
    memcpy(a4988_driver_config, config, sizeof(a4988_config_t));

    ledc_timer = (ledc_timer_config_t*)malloc(sizeof(ledc_timer_config_t));
    ESP_GOTO_ON_FALSE(ledc_timer, ESP_ERR_NO_MEM, a4988_out, TAG, "no mem for ledc_timer_config_t");
    memset(ledc_timer, 0, sizeof(ledc_timer_config_t));

    // Initialize LEDC timer configuration
    ledc_timer->speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_timer->clk_cfg = LEDC_AUTO_CLK;
    ledc_timer->timer_num = LEDC_TIMER_0;
    ledc_timer->freq_hz = 32;
    ledc_timer->duty_resolution = LEDC_TIMER_5_BIT;

    ret = ledc_timer_config(ledc_timer);
    ESP_GOTO_ON_ERROR(ret, a4988_out, TAG, "ledc_timer_config");

    // Allocate memory for the channel configuration struct
    ledc_channel = (ledc_channel_config_t*)malloc(sizeof(ledc_channel_config_t));
    ESP_GOTO_ON_FALSE(ledc_channel, ESP_ERR_NO_MEM, a4988_out, TAG, "no mem for ledc_channel_config_t");
    memset(ledc_channel, 0, sizeof(ledc_channel_config_t));

    ledc_channel->timer_sel = LEDC_TIMER_0;
    ledc_channel->speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_channel->channel = LEDC_CHANNEL_0;
    ledc_channel->duty = 0;
    ledc_channel->hpoint = 0;
    ledc_channel->gpio_num = a4988_driver_config->step_pin;

    ret = ledc_channel_config(ledc_channel);
    ESP_GOTO_ON_ERROR(ret, a4988_out, TAG, "ledc_channel_config");

    gpio_config_t direction_pin_io_conf = {
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .pin_bit_mask = (1ULL << a4988_driver_config->direction_pin),
    };
    ret = gpio_config(&direction_pin_io_conf);
    ESP_GOTO_ON_ERROR(ret, a4988_out, TAG, "invalid gpio direction_pin_io_conf");

    gpio_config_t en_pin_io_conf = {
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .pin_bit_mask = (1ULL << a4988_driver_config->en_pin),
    };
    ret = gpio_config(&en_pin_io_conf);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
    
    a4988_driver_status = A4988_INITIALIZED;
    return ESP_OK;

    a4988_out:
    free(a4988_driver_config);
    free(ledc_timer);
    free(ledc_channel);
    a4988_driver_config = NULL;
    ledc_timer = NULL;
    ledc_channel = NULL;
    return ESP_FAIL;
}

void a4988_set_direction(a4988_direction_t direction)
{
    gpio_set_level(a4988_driver_config->direction_pin, direction);
}

void a4988_enable_driver()
{
    gpio_set_level(a4988_driver_config->en_pin, 0);
}

void a4988_disable_driver()
{
    gpio_set_level(a4988_driver_config->en_pin, 1);
}

void a4988_set_speed(float rpm)
{
    uint32_t frequency = ((rpm/60.f) * a4988_driver_config->steps_per_rev);
    if (frequency)
    {  
        ledc_set_freq(ledc_timer->speed_mode, ledc_timer->timer_num, frequency);
        ledc_set_duty(ledc_channel->speed_mode, ledc_channel->channel, DUTY_CYCLE_50PERCENT_VALUE);
        ledc_update_duty(ledc_channel->speed_mode, ledc_channel->channel);
    }
    else
    {
        ledc_set_duty(ledc_channel->speed_mode, ledc_channel->channel, 0);
        ledc_update_duty(ledc_channel->speed_mode, ledc_channel->channel);
    }
}

void a4988_run(float rpm, a4988_direction_t direction)
{
    a4988_set_direction(direction);
    a4988_set_speed(rpm);
}

bool a4988_is_initialized()
{
    if (A4988_INITIALIZED == a4988_driver_status)
    {
        return true;
    }
    return false;
}