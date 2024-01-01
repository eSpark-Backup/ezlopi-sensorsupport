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
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 100 / portTICK_RATE_MS);
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
        // char *string = cJSON_Print(root);

        cJSON* seg = cJSON_GetObjectItemCaseSensitive(root, "seg");

        cJSON* fx = cJSON_GetObjectItemCaseSensitive(seg, "fx");
        if(fx != NULL){
            config_setMode((uint8_t)fx->valueint);
        }

        cJSON* sx = cJSON_GetObjectItemCaseSensitive(seg, "sx");
        if(sx != NULL){
            config_set_effect_speed((uint8_t)sx->valueint);
        }

        cJSON* ix = cJSON_GetObjectItemCaseSensitive(seg, "ix");
        if(ix != NULL){
            config_set_effect_intensity((uint8_t)ix->valueint);
        }

        cJSON* col = cJSON_GetObjectItemCaseSensitive(seg, "col");
        if(col != NULL){
            for (int i = 0 ; i < cJSON_GetArraySize(col) ; i++)
            {
                    cJSON* subitem = cJSON_GetArrayItem(col, i);

                    for(int j = 0; j < cJSON_GetArraySize(subitem)-1; j++ ){ //FIXME: Array size is 4 RGBW not RGB
                        if(cJSON_GetArraySize(subitem) != 0)
                        {
                            cJSON* subsubitem = cJSON_GetArrayItem(subitem, j);
                            ESP_LOGI(TAG, "JSON COL %d : %u",j, subsubitem->valueint);
                            uint8_t color = subsubitem->valueint;
                            rgbcolor[i] += (color << ((2-j)*8));
                        }
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
                if(start == NULL && stop == NULL){
                    
                }
                else if((start != NULL) & (stop != NULL)){
                    int seg_start, seg_stop;
                    seg_start = start->valueint;
                    seg_stop = stop->valueint;
                    ESP_LOGI(TAG, "Segment Start: %d", seg_start);
                    config_set_segment_start_stop(seg_id, seg_start, seg_stop);
                }

            }


            cJSON* rev = cJSON_GetObjectItemCaseSensitive(seg, "rev");
            if(NULL != rev){
                // config_set_reverse_option(id->valueint, cJSON_IsTrue(rev));
                config_set_option(id->valueint, cJSON_IsTrue(rev), 0x80);
            }

            cJSON* mirror =  cJSON_GetObjectItemCaseSensitive(seg, "mir");
            
            cJSON* delete = cJSON_GetObjectItemCaseSensitive(seg, "stop");
            if((NULL != delete) && (NULL == n)){
                ESP_LOGI(TAG, "Delete Json parsing working");
                config_delete_segment(id->valueint);
            }

        }

        cJSON* pal = cJSON_GetObjectItemCaseSensitive(seg, "pal");
        if(pal != NULL){
            config_apply_palette(pal->valueint);
        }

        cJSON* bri = cJSON_GetObjectItemCaseSensitive(root, "bri");
        if(bri != NULL){
            ESP_LOGW(TAG, "Setting Brightness : %d", bri->valueint);
            config_set_brightness(bri->valueint);
        }

        
    }
    cJSON_Delete(root);
    // memset(start, 0 , BUF_SIZE);
}


char* updated_config_json()
{
    // Create the outer cJSON object
    cJSON *root = cJSON_CreateObject();

    // Create and add "state" object
    cJSON *state = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "state", state);

    // Add properties to "state" object
    cJSON_AddBoolToObject(state, "on", true);
    cJSON_AddNumberToObject(state, "bri", 128);
    cJSON_AddNumberToObject(state, "transition", 7);

    // Create and add "nl" object
    cJSON *nl = cJSON_CreateObject();
    cJSON_AddItemToObject(state, "nl", nl);

    // Add properties to "nl" object
    cJSON_AddBoolToObject(nl, "on", false);
    cJSON_AddNumberToObject(nl, "dur", 60);
    cJSON_AddNumberToObject(nl, "mode", 1);
    cJSON_AddNumberToObject(nl, "tbri", 0);
    cJSON_AddNumberToObject(nl, "rem", -1);

    // Create and add "udpn" object
    cJSON *udpn = cJSON_CreateObject();
    cJSON_AddItemToObject(state, "udpn", udpn);

    // Add properties to "udpn" object
    cJSON_AddBoolToObject(udpn, "send", false);
    cJSON_AddBoolToObject(udpn, "recv", true);
    cJSON_AddNumberToObject(udpn, "sgrp", 1);
    cJSON_AddNumberToObject(udpn, "rgrp", 1);

    // Create and add "seg" array
    cJSON *seg_array = cJSON_CreateArray();
    cJSON_AddItemToObject(state, "seg", seg_array);

    // Create and add a "seg" object within the array
    cJSON *seg_object = cJSON_CreateObject();
    cJSON_AddItemToArray(seg_array, seg_object);

    // Add properties to the "seg" object
    cJSON_AddNumberToObject(seg_object, "id", 0);
    cJSON_AddNumberToObject(seg_object, "start", 0);
    cJSON_AddNumberToObject(seg_object, "stop", 30);

    // Create and add "col" array to "seg" object
    cJSON *col_array = cJSON_CreateArray();
    cJSON_AddItemToObject(seg_object, "col", col_array);

    // Add individual arrays to "col" array
    cJSON *col_element_1 = cJSON_CreateIntArray((const int[]){255, 160, 0}, 3);
    cJSON *col_element_2 = cJSON_CreateIntArray((const int[]){0, 0, 0}, 3);
    cJSON *col_element_3 = cJSON_CreateIntArray((const int[]){0, 0, 0}, 3);

    cJSON_AddItemToArray(col_array, col_element_1);
    cJSON_AddItemToArray(col_array, col_element_2);
    cJSON_AddItemToArray(col_array, col_element_3);

    cJSON_AddNumberToObject(seg_object, "fx", 1);
    cJSON_AddNumberToObject(seg_object, "sx", 128);
    cJSON_AddNumberToObject(seg_object, "ix", 128);
    cJSON_AddNumberToObject(seg_object, "pal", 0);
    cJSON_AddBoolToObject(seg_object, "sel", true);
    cJSON_AddBoolToObject(seg_object, "rev", false);
    cJSON_AddBoolToObject(seg_object, "mi", false);
    



    // Create and add "info" object
    cJSON *info = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "info", info);

    // Add properties to "info" object
    cJSON_AddStringToObject(info, "ver", "0.14.0");
    cJSON_AddNumberToObject(info, "vid", 2310130);

    // Create and add "leds" object to "info" object
    cJSON *leds = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "leds", leds);

    // Add properties to "leds" object
    cJSON_AddNumberToObject(leds, "count", 100);
    cJSON_AddNumberToObject(leds, "pwr", 480);
    cJSON_AddNumberToObject(leds, "fps", 4);
    cJSON_AddNumberToObject(leds, "maxpwr", 850);
    // Create and add "seglc" array to "leds" object
    cJSON *seglc_array = cJSON_CreateArray();
    cJSON_AddItemToObject(leds, "seglc", seglc_array);

    // Add a single element to "seglc" array
    cJSON_AddNumberToObject(seglc_array, "", 1);
    // ... add other properties to "leds" object ...

    cJSON_AddFalseToObject(info, "str"); // Example for boolean value
    cJSON_AddStringToObject(info, "name", "WLED");
    cJSON_AddNumberToObject(info, "udpport", 21324);
    cJSON_AddFalseToObject(info, "live"); // Example for boolean value
    cJSON_AddNumberToObject(info, "liveseg", -1);
    cJSON_AddStringToObject(info, "lm", "");
    cJSON_AddStringToObject(info, "lip", "");
    cJSON_AddNumberToObject(info, "ws", 0);
    cJSON_AddNumberToObject(info, "fxcount", 187);
    cJSON_AddNumberToObject(info, "palcount", 71);
    cJSON_AddNumberToObject(info, "cpalcount", 0);

    // Create and add "maps" array to "info" object
    cJSON *maps_array = cJSON_CreateArray();
    cJSON_AddItemToObject(info, "maps", maps_array);

    // Create and add a "maps" object within the array
    cJSON *maps_object = cJSON_CreateObject();
    cJSON_AddItemToArray(maps_array, maps_object);

    cJSON_AddNumberToObject(maps_object, "id", 0);

    // Create and add "wifi" object to "info" object
    cJSON *wifi = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "wifi", wifi);

    // Add properties to "wifi" object
    cJSON_AddStringToObject(wifi, "bssid", "6C:44:2A:D4:03:C4");
    cJSON_AddNumberToObject(wifi, "rssi", -67);
    cJSON_AddNumberToObject(wifi, "signal", 66);
    cJSON_AddNumberToObject(wifi, "channel", 5);

    // Create and add "fs" object to "info" object
    cJSON *fs = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "fs", fs);

    // Add properties to "fs" object
    cJSON_AddNumberToObject(fs, "u", 12);
    cJSON_AddNumberToObject(fs, "t", 983);
    cJSON_AddNumberToObject(fs, "pmt", 0);

    cJSON_AddNumberToObject(info, "ndc", 0);
    cJSON_AddStringToObject(info, "arch", "esp32");
    cJSON_AddStringToObject(info, "core", "v3.3.6-16-gcc5440f6a2");
    cJSON_AddNumberToObject(info, "lwip", 0);
    cJSON_AddNumberToObject(info, "freeheap", 198056);
    cJSON_AddNumberToObject(info, "uptime", 85);
    cJSON_AddStringToObject(info, "time", "1970-1-1, 00:01:25");
    cJSON_AddNumberToObject(info, "opt", 79);
    cJSON_AddStringToObject(info, "brand", "WLED");
    cJSON_AddStringToObject(info, "product", "FOSS");
    cJSON_AddStringToObject(info, "mac", "94e68604985c");
    cJSON_AddStringToObject(info, "ip", "192.168.18.151");

    // Print the generated JSON
    char *json_string = cJSON_Print(root);
    cJSON_Minify(json_string);

    // Free the cJSON object
    cJSON_Delete(root);
    return json_string;

}