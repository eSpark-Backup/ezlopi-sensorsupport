
#include"sensor_0066_tfmini_lidar_range_module.h"
#include "trace.h"
#include"ezlopi_uart.h"


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
            vTaskDelay(100/portTICK_PERIOD_MS); // 10 ms , portTICK_PERIOD_MS = 10 
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

