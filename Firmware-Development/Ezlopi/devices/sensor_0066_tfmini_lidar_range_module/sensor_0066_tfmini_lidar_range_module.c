
#include"sensor_0066_tfmini_lidar_range_module.h"
#include "trace.h"
#include"ezlopi_uart.h"

// extern uint8_t buffer[TFMINI_BUFFER_SIZE];
static const char* TAG = "TFMINIS";

// Declaration of ezlopi_tfmini_upcall
static void ezlopi_tfmini_upcall(uint8_t* buffer, s_ezlopi_uart_object_handle_t s_ezlopi_uart_object, void* user_args);

// static void ezlopi_tfmini_upcall(uint8_t buffer, s_ezlopi_uart_object_handle_t s_ezlopi_uart_object, void* user_args);
void tfmini_uart_setup(TFMINI_Data* tfmini_sensor_data_structure, uint32_t tx, uint32_t rx, uint32_t baudrate)
{
    TRACE_B("UART setup!!");
    ezlopi_uart_init(baudrate, tx, rx, ezlopi_tfmini_upcall, tfmini_sensor_data_structure); 
}

static void ezlopi_tfmini_upcall(uint8_t* buffer, s_ezlopi_uart_object_handle_t s_ezlopi_uart_object, void* user_args)
{
     TFMINI_Data* tfmini_sensor_data_structure = (TFMINI_Data*)user_args;
     tfmini_read_upcall(buffer,tfmini_sensor_data_structure);
}

esp_err_t tfmini_read_upcall(uint8_t* buffer, TFMINI_Data* data)
{
    if (buffer == NULL)
    {
        // Handle NULL pointer gracefully
        return ESP_ERR_INVALID_ARG;
    }

    // Check if the buffer contains at least 8 elements
    if (sizeof(buffer) < 8)
    {
        // Handle insufficient buffer size gracefully
        return ESP_ERR_INVALID_SIZE;
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

            return ESP_OK;
        }
        else
        {
            // Handle invalid values in buffer gracefully
            return ESP_ERR_INVALID_STATE;
        }
    }
    else
    {
        // Handle unexpected header values gracefully
        return ESP_ERR_INVALID_RESPONSE;
    }
}

void tfmini_parse_and_assign(uint8_t* buffer, TFMINI_Data* data)
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
            vTaskDelay(1/10);
            // Print the results for debugging
            // printf("tfmini_distance: %d\n", data->tfmini_distance);
            // printf("tfmini_strength: %d\n", data->tfmini_strength);
            // printf("tfmini_chip_temperature: %d\n", data->tfmini_chip_temperature);
        }
        else
        {
            // Handle invalid values in buffer gracefully
            printf("Error: Invalid values in buffer in tfmini_parse_and_assign\n");
        }
    }
    // else
    //     {
    //         // Handle invalid values in buffer gracefully
            // printf("Error: Header mismatch\n");
    //     }

}

void tfmini_print_data(TFMINI_Data* data)
{
    TRACE_B("Reading success");
    TRACE_B("TFMINI:  distance  :%d", data->tfmini_distance);
    TRACE_B("TFMINI: temperature :%d", data->tfmini_chip_temperature);
    TRACE_B("TFMINI:   strength :%d", data->tfmini_strength);
}

