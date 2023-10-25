// Copyright 2022  William Henrique (william.martins@ee.ufcg.edu.br)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file keypad.c
 * @author William Henrique (william.martins@ee.ufcg.edu.br)
 * @brief 4x4 Keypad library for ESP-IDF framework
 * @version 0.1
 * @date 2022-07-30
 */

#include "keypad.h"

#include <memory.h>
#include <time.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static uint8_t _rows = 0;
static uint8_t _cols = 0;
static uint8_t _total_gpios = 0;

/** \brief Keypad mapping array*/
const char keypad4x4[] = { 
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D'
};
/** \brief Keypad mapping array*/
const char keypad4x3[] = { 
    '1', '2', '3', 
    '4', '5', '6', 
    '7', '8', '9', 
    '*', '0', '#'
};

/** \brief Keypad mapping array*/
const char keypad1x4[] = { 
    '1', '2', '3', '4'
};

/** \brief Keypad mapping array*/
const char keypad_general[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

const char* keypad = NULL;

/** \brief Keypad configuration pions*/
static gpio_num_t* _keypad_pins;

/** \brief Last isr time*/
time_t time_old_isr = 0;

/** \brief Pressed keys queue*/
QueueHandle_t keypad_queue;

/**
 * @brief Handle keypad click
 * @param [in]args row number
 */
void intr_click_handler(void *args);

/**
 * @brief Enable rows'pin pullup resistor, and isr. Prepares
 * keypad to read pressed row number.
 */
void turnon_rows()
{
    for(int i = _rows; i < _total_gpios; i++) /// Columns
    {
        gpio_set_pull_mode(_keypad_pins[i], GPIO_PULLDOWN_ONLY);
    }
    for(int i = 0; i < _rows; i++) /// Rows
    {
        gpio_set_pull_mode(_keypad_pins[i], GPIO_PULLUP_ONLY);
        gpio_intr_enable(_keypad_pins[i]);
    }
}

/**
 * @brief Enable columns'pin pullup resistor, and disable rows isr and pullup resistor.
 * Prepares keypad to read pressed column number. 
 */
void turnon_cols()
{
    for(int i = 0; i < _rows; i++) /// Rows
    {
        gpio_intr_disable(_keypad_pins[i]);
        gpio_set_pull_mode(_keypad_pins[i], GPIO_PULLDOWN_ONLY);
    }
    for(int i = _rows; i < _total_gpios; i++) /// Columns
    {
        gpio_set_pull_mode(_keypad_pins[i], GPIO_PULLUP_ONLY);
    }
}

esp_err_t keypad_initalize(gpio_num_t keypad_pins[], uint8_t rows, uint8_t cols)
{
    _rows = rows;
    _cols = cols;
    _total_gpios = rows + cols;

    // Create a lookup table for the keypad configurations
    keypad_config_t keypad_configs[] = {
        {4, 4, keypad4x4},
        {4, 3, keypad4x3},
        {1, 4, keypad1x4},
    };

    // Default to the general keypad configuration
    keypad = keypad_general;

    // Find the corresponding configuration in the lookup table
    for (uint8_t i = 0; i < sizeof(keypad_configs) / sizeof(keypad_config_t); i++) {
        if (keypad_configs[i].rows == _rows && keypad_configs[i].cols == _cols) {
            keypad = keypad_configs[i].mapping_layout;
            break;
        }
    }

    _keypad_pins = keypad_pins;
    // memcpy(_keypad_pins, keypad_pins, _total_gpios * sizeof(gpio_num_t));

    /** Maybe cause issues if try to desinstall this flag because it's global allocated 
     * to all pins try to use gpio_isr_register instrad of gpio_install_isr_service **/
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_install_isr_service(ESP_INTR_FLAG_EDGE));
    for(int i = 0; i < _rows; i++) /// Rows
    {
        gpio_intr_disable(keypad_pins[i]);
        gpio_set_direction(keypad_pins[i], GPIO_MODE_INPUT);
        gpio_set_intr_type(keypad_pins[i], GPIO_INTR_NEGEDGE);
        ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_isr_handler_add(_keypad_pins[i], (void*)intr_click_handler, (void*)i));
        
    }
    for(int i = _rows; i < _total_gpios; i++)
    {
        gpio_set_direction(keypad_pins[i], GPIO_MODE_INPUT);
    }

    keypad_queue = xQueueCreate(20, sizeof(char));
    if(keypad_queue == NULL)
        return ESP_ERR_NO_MEM;

    turnon_rows();

    return ESP_OK;
}

void intr_click_handler(void* args)
{
    int index = (int)(args);
    
    time_t time_now_isr = esp_timer_get_time()/1000L;
    time_t time_isr = (time_now_isr - time_old_isr);
    
        if(time_isr >= KEYPAD_DEBOUNCING)
    {
        turnon_cols();
        for(int j = _rows; j < _total_gpios; j++)
        {
            if(!gpio_get_level(_keypad_pins[j]))
            {
                xQueueSendFromISR(keypad_queue, &keypad[index*_cols + j - _rows], NULL);
                break;
            }
        }
        turnon_rows();
    }
    time_old_isr = time_now_isr;
    
}

char keypad_getkey()
{
    char key = '\0';
    if(!uxQueueMessagesWaiting(keypad_queue)) /// if is empty, return teminator character
        return key;

    xQueueReceive(keypad_queue, &key, 10/portTICK_RATE_MS);    
    return key;
}

void keypad_delete()
{
    for(int i = 0; i < _total_gpios; i++)
    {   
        gpio_isr_handler_remove(_keypad_pins[i]);
        gpio_set_direction(_keypad_pins[i], GPIO_MODE_DISABLE);
    }
    vQueueDelete(keypad_queue);
}