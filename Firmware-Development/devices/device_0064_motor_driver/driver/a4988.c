#include "a4988.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include <string.h>
#include "freertos/queue.h"
#include "driver/pcnt.h"
#include "esp_attr.h"
#include "esp_log.h"

static const char* TAG = "a4988 driver";
xQueueHandle pcnt_evt_queue = NULL;   // A queue to handle pulse counter events
static void stepperStopTask(void *pvParameters);

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
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .pin_bit_mask = (1ULL << a4988_driver_config->direction_pin),
    };
    ret = gpio_config(&direction_pin_io_conf);
    ESP_GOTO_ON_ERROR(ret, a4988_out, TAG, "invalid gpio direction_pin_io_conf");

    gpio_config_t en_pin_io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .pin_bit_mask = (1ULL << a4988_driver_config->en_pin),
    };
    ret = gpio_config(&en_pin_io_conf);
    ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
    
    if (a4988_driver_config->micro_step_mode)
    {
        gpio_config_t ms_io_conf = 
        {
            .mode = GPIO_MODE_OUTPUT,
            .intr_type = GPIO_INTR_DISABLE,
            .pull_down_en = 0,
            .pull_up_en = 0,
            .pin_bit_mask = (1ULL << a4988_driver_config->ms1_pin) | (1ULL << a4988_driver_config->ms2_pin) | (1ULL << a4988_driver_config->ms3_pin),
        };
        ret = gpio_config(&ms_io_conf);
        ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
    }

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
        ledc_set_duty(ledc_channel->speed_mode, ledc_channel->channel, DUTY_CYCLE_50_PERCENT_VAL);
        ledc_update_duty(ledc_channel->speed_mode, ledc_channel->channel);
    }
    else
    {
        ledc_set_duty(ledc_channel->speed_mode, ledc_channel->channel, 0);
        ledc_update_duty(ledc_channel->speed_mode, ledc_channel->channel);
    }
}

bool a4988_run(motor_run_properties_t* motor_properties)
{
    if ( !motor_properties->rpm || !motor_properties->steps)
        return false;

    a4988_set_direction(motor_properties->direction);
    a4988_set_speed(motor_properties->rpm);
    a4988_set_steps(motor_properties->steps);
    return true;
}

bool a4988_is_initialized()
{
    if (A4988_INITIALIZED == a4988_driver_status)
    {
        return true;
    }
    return false;
}

static void IRAM_ATTR pcnt_isr(void *arg)
{
    int pcnt_unit = (int)arg;
    pcnt_evt_t evt;
    evt.unit = pcnt_unit;
    /* Save the PCNT event type that caused an interrupt
       to pass it to the main program */
    pcnt_get_event_status(pcnt_unit, &evt.status);
    xQueueSendFromISR(pcnt_evt_queue, &evt, NULL);
}

void a4988_set_steps(uint32_t steps)
{
    int unit = PCNT_UNIT_0;

    a4988_driver_config->steps_left = steps;  
    a4988_driver_config->steps = a4988_driver_config->steps_left >= PCNT_H_LIM_VAL ? PCNT_H_LIM_VAL - 1 : a4988_driver_config->steps_left;
    ESP_LOGE(TAG, "set counter %d", a4988_driver_config->steps);

    /* Prepare configuration for the PCNT unit */
    pcnt_config_t pcnt_config = {
        // Set PCNT input signal and control GPIOs
        .pulse_gpio_num = a4988_driver_config->counter_gpio,
        .ctrl_gpio_num = PCNT_INPUT_CTRL_IO,
        .channel = PCNT_CHANNEL_0,
        .unit = unit,
        // What to do on the positive / negative edge of pulse input?
        .pos_mode = PCNT_COUNT_INC,   // Count up on the positive edge
        .neg_mode = PCNT_COUNT_DIS,   // Keep the counter value on the negative edge
        // What to do when control input is low or high?
        .lctrl_mode = PCNT_MODE_KEEP, // Reverse counting direction if low
        .hctrl_mode = PCNT_MODE_KEEP,    // Keep the primary counter mode if high
        // Set the maximum and minimum limit values to watch
        .counter_h_lim = a4988_driver_config->steps,
        .counter_l_lim = PCNT_L_LIM_VAL,
    };
    /* Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);

    pcnt_event_enable(unit, PCNT_EVT_H_LIM);

    /* Initialize PCNT's counter */
    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);

    /* Install interrupt service and add isr callback handler */
    pcnt_isr_service_install(0);
    pcnt_isr_handler_add(unit, pcnt_isr, (void *)unit);

    /* Everything is set up, now go to counting */
    pcnt_counter_resume(unit);

    // // Workaround
    // if((steps > (PCNT_H_LIM_VAL-1)) && (steps <((PCNT_H_LIM_VAL-1) * 2U)))
    //     pcnt_set_event_value(unit, PCNT_EVT_H_LIM, steps - (PCNT_H_LIM_VAL-1));

    if (NULL == pcnt_evt_queue)
    {
        pcnt_evt_queue = xQueueCreate(3, sizeof(pcnt_evt_t));
        xTaskCreate(stepperStopTask, "stepperStopTask", 2048, NULL, configMAX_PRIORITIES - 1, NULL);
    }
}

static void stepperStopTask(void *pvParameters) 
{
    pcnt_evt_t evt;
    int16_t count = 0;
    portBASE_TYPE res;
    while (1) 
    {
        res = xQueueReceive(pcnt_evt_queue, &evt, 500/portTICK_RATE_MS);
        if (res == pdTRUE) 
        {
            pcnt_get_counter_value(evt.unit, &count);
            if (evt.status & PCNT_EVT_H_LIM) 
            {
                a4988_driver_config->steps_left -= a4988_driver_config->steps;

                if (a4988_driver_config->steps_left > 0)
                {
                    // a4988_driver_config->steps = a4988_driver_config->steps_left >= PCNT_H_LIM_VAL ? PCNT_H_LIM_VAL - 1 : a4988_driver_config->steps_left;
                    // // Workaround
                    // uint32_t last_steps = a4988_driver_config->steps_left - a4988_driver_config->steps;
                    // if (last_steps < PCNT_H_LIM_VAL)
                    // {
                    //     pcnt_set_event_value(evt.unit, PCNT_EVT_H_LIM, last_steps);
                    // }
                    a4988_set_steps(a4988_driver_config->steps_left);
                }
                else
                {
                    a4988_driver_config->steps = 0;
                    a4988_driver_config->steps_left = 0;

                    ESP_LOGI(TAG, "Steps Completed");
                    ESP_LOGI(TAG, "counter %d", count);
                    a4988_set_speed(0);
                }
            }
        }
        pcnt_get_counter_value(PCNT_UNIT_0, &count);
        ESP_LOGI(TAG, "counter %d", count);
    }
}