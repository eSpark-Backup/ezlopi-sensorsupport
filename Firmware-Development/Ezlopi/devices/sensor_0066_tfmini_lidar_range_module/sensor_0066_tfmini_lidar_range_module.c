
#include"sensor_0066_tfmini_lidar_range_module.h"
#include "trace.h"
#include"ezlopi_uart.h"

extern uint8_t buffer[TFMINI_BUFFER_SIZE];
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
    for (int i =0 ; i<9; i++)
        {
            if (buffer[i] == 0x59 && buffer[i+1] == 0x59)
            {
                data->tfmini_distance = buffer[i+2] + buffer[i+3]*255; 
                data->tfmini_strength = buffer[i+4] + buffer[i+5]*255 ; 
                data->tfmini_chip_temperature = (buffer[i+6] + buffer[i+7]*255)/ 8 - 256; 
                vTaskDelay(pdMS_TO_TICKS(100)); // 100 milliseconds = 0.1 second
                return ESP_OK;
            }
        }

    return ESP_FAIL;
    
}

void tfmini_print_data(TFMINI_Data* data)
{
    TRACE_B("Reading success");
    TRACE_B("TFMINI:  distance  :%d", data->tfmini_distance);
    TRACE_B("TFMINI: temperature :%d", data->tfmini_chip_temperature);
    TRACE_B("TFMINI:   strength :%d", data->tfmini_strength);
}

