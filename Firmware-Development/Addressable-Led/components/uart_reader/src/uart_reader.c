/* UART Select Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "uart_reader.h"

static const char* TAG = "uart_select_example";
void parse_json(char* start);
char c[1024];

#define ECHO_TEST_TXD 1
#define ECHO_TEST_RXD 3
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      UART_NUM_0
#define ECHO_UART_BAUD_RATE     115200
#define ECHO_TASK_STACK_SIZE    2048

#define BUF_SIZE (1024)

void uart_task(void *arg)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_RATE_MS);
        // Write data back to the UART
        // uart_write_bytes(ECHO_UART_PORT_NUM, (const char *) data, len);

        if (len) {
            data[len] = '\0';
            ESP_LOGI(TAG, "Recv str: %s", (char *) data);
            parse_json((char*)data);
        }
    }
}

void parse_json(char* start){
    cJSON* root = cJSON_Parse(start);
    uint32_t rgbcolor[3] = {0};
    if(root != NULL){
        char *string = cJSON_Print(root);

        cJSON* seg = cJSON_GetObjectItemCaseSensitive(root, "seg");

        cJSON* fx = cJSON_GetObjectItemCaseSensitive(seg, "fx");
        if(fx != NULL){
            config_setMode((uint8_t)fx->valueint);
        }

        cJSON* col = cJSON_GetObjectItemCaseSensitive(seg, "col");
        if(col != NULL){
            for (int i = 0 ; i < cJSON_GetArraySize(col) ; i++)
            {
                    cJSON* subitem = cJSON_GetArrayItem(col, i);

                    for(int j = 0; j < cJSON_GetArraySize(subitem)-1; j++ ){ //FIXME: Array size is 4 RGBW not RGB
                        cJSON* subsubitem = cJSON_GetArrayItem(subitem, j);
                        ESP_LOGI(TAG, "JSON COL %d : %u",j, subsubitem->valueint);
                        uint8_t color = subsubitem->valueint;
                        rgbcolor[i] += (color << ((2-j)*8));
                    }
            }
        	config_setColor(rgbcolor);
        }

        cJSON* id = cJSON_GetObjectItemCaseSensitive(seg, "id");
        if(id != NULL){
            //set the selected flag of segment to true or false
            cJSON* sel = cJSON_GetObjectItemCaseSensitive(seg, "sel");
            if(sel != NULL){
                ESP_LOGI(TAG, "SEG id : %d is selected %d", id->valueint, cJSON_IsTrue(sel));
                config_set_selected_flag(id->valueint, cJSON_IsTrue(sel));
            }
            cJSON* n = cJSON_GetObjectItemCaseSensitive(seg, "n");
            if(n != NULL){
                ESP_LOGI(TAG, "Name of Segment : %s", n->valuestring);
                int seg_id = id->valueint;
                cJSON* start = cJSON_GetObjectItemCaseSensitive(seg, "start");
                cJSON* stop = cJSON_GetObjectItemCaseSensitive(seg, "stop");
                if((start != NULL) & (stop != NULL)){
                    int seg_start, seg_stop;
                    seg_start = start->valueint;
                    seg_stop = stop->valueint;
                    ESP_LOGI(TAG, "Segment Start: %d", seg_start);
                    config_set_segment_start_stop(seg_id, seg_start, seg_stop);
                }

            }
        }

        cJSON* bri = cJSON_GetObjectItemCaseSensitive(root, "bri");
        if(bri != NULL){
            config_set_brightness(bri->valueint);
        }

        
    }
    cJSON_Delete(root);
    memset(start, 0 , BUF_SIZE);
}


