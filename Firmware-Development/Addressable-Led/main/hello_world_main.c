/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "led.h"
#include "status.h"
#include "config.h"
#include "uart_reader.h"




void app_main(void)
{
    xTaskCreate(uart_select_task, "uart_select_task", 4*1024, NULL, 5, NULL);
    printf("Hello world!\n");
	status_init();
    config_init();
    led_on();
	config_update_channels(); 	

    while (1)
    {
        printf("Hello world!\n");
        vTaskDelay(10000/portTICK_PERIOD_MS);
    }

}