
#include"tfmini_lidar_range_module.h"
#include "trace.h"
#include"ezlopi_uart.h"

static void tfmini_upcall(uint8_t* buffer, s_ezlopi_uart_object_handle_t s_ezlopi_uart_object);
static void tfmini_parse_and_assign(uint8_t* buffer,tfmini_data_t* data);

s_ezlopi_uart_object_handle_t tfmini_init(uint32_t baudrate, uint32_t tx, uint32_t rx, tfmini_data_t* tfmini_sensor_data_structure)
{
    s_ezlopi_uart_object_handle_t ezlopi_uart_object_handle = ezlopi_uart_init(baudrate, tx, rx, tfmini_upcall, tfmini_sensor_data_structure);
    return ezlopi_uart_get_channel(ezlopi_uart_object_handle);
}

static void tfmini_upcall(uint8_t* buffer, s_ezlopi_uart_object_handle_t uart_object_handle)
{
    tfmini_data_t* tfmini_sensor_data_structure = (tfmini_data_t*)uart_object_handle->arg;
    tfmini_parse_and_assign(buffer, tfmini_sensor_data_structure);
}

static void tfmini_parse_and_assign(uint8_t* buffer, tfmini_data_t* data)
{    
    if (buffer == NULL || data == NULL)
    {
        // Handle NULL pointers gracefully
        printf("Error: NULL pointer detected in tfmini_parse_and_assign\n");
        return;
    }

    // Ensure buffer indices are within bounds
    if ((buffer[0] == 0x59 && buffer[1] == 0x59) || (buffer[1] == 0x59 && buffer[2] == 0x59) || (buffer[2] == 0x59 && buffer[3] == 0x59))
    {
        if (buffer[2] < 255 && buffer[3] < 255 &&
            buffer[4] < 255 && buffer[5] < 255 &&
            buffer[6] < 255 && buffer[7] < 255)
        {
            data->tfmini_distance = buffer[2] + buffer[3] * 255;
            data->tfmini_strength = buffer[4] + buffer[5] * 255;
            data->tfmini_chip_temperature = (buffer[6] + buffer[7] * 255) / 8 - 256;
        }
        else
        {
            // Handle invalid values in buffer gracefully
            printf("Error: Invalid values in buffer in tfmini_parse_and_assign\n");
        }
    }
    vTaskDelay(50/portTICK_PERIOD_MS); 
}

