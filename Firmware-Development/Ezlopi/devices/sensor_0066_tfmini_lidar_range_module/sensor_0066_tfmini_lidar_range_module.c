
#include"sensor_0066_tfmini_lidar_range_module.h"
#include "trace.h"
#include"ezlopi_uart.h"

extern uint8_t buffer[TFMINI_BUFFER_SIZE];
static const char* TAG = "TFMINIS";

// Declaration of ezlopi_tfmini_upcall
static void ezlopi_tfmini_upcall(uint8_t* buffer, s_ezlopi_uart_object_handle_t s_ezlopi_uart_object, void* user_args);

// static void ezlopi_tfmini_upcall(uint8_t buffer, s_ezlopi_uart_object_handle_t s_ezlopi_uart_object, void* user_args);
void tfmini_uart_setup(TFMINI_sensor_structure_t* tfmini_sensor_structure, uint32_t tx, uint32_t rx, uint32_t baudrate)
{
    TRACE_B("UART setup!!");
    ezlopi_uart_init(baudrate, tx, rx, ezlopi_tfmini_upcall, &tfmini_sensor_structure->tfmini_data); 
}

static void ezlopi_tfmini_upcall(uint8_t* buffer, s_ezlopi_uart_object_handle_t s_ezlopi_uart_object, void* user_args)
{
     TFMINI_sensor_structure_t* tfmini_sensor_structure = (TFMINI_sensor_structure_t*)user_args;
     tfmini_read_upcall(buffer,tfmini_sensor_structure->tfmini_data);
}

esp_err_t tfmini_read_upcall(uint8_t* buffer, TFMINI_Data* data)
{
    data = (TFMINI_Data*)malloc(sizeof(TFMINI_Data));
    memset(data,0,sizeof(TFMINI_Data));

    for (int i =0 ; i<8; i++)
    {
        if (buffer[i] == 0x59 && buffer[i+1] == 0x59)
        {
            data->tfmini_distance = buffer[i+2] + buffer[i+3]*255; // Distance
            data->tfmini_strength = buffer[i+4] + buffer[i+5]*255 ; // Signal Strength
            data->tfmini_chip_temprature = (buffer[i+6] + buffer[i+7]*255)/ 8 - 256; // Temperature
            free(data);
            ESP_LOGI(TAG, "ESP_OK");
            return ESP_OK;
        }
        else
          continue;
    }
    ESP_LOGE(TAG, "Invalid data from TFMini");
    return ESP_FAIL;
    
}

void tfmini_print_data(TFMINI_Data* data)
{
    TRACE_B("Reading success");
    TRACE_B("TFMINI: %d", data->tfmini_distance);
    TRACE_B("TFMINI: %d", data->tfmini_chip_temprature);
    TRACE_B("TFMINI:  %d", data->tfmini_strength);
}

