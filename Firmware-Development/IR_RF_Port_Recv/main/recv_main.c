/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "esp_sleep.h"

#define ON_SYNC_WORD_BYTE 211
#define WAKE_UP_SYNC_WORD_BYTE 212
#define SHUTDOWN_SYNC_WORD_BYTE 213
#define NUM_TRIGGERS 3


static const char *TAG = "RECIEVER";
byte buffer[61] = {0};


enum TRIGGER {
    SHUTDOWN,
    POWERUP,
    SLEEP
};

typedef struct{
    const char* methodstr;
    uint8_t method;
}members;

members obj[NUM_TRIGGERS] = 
{
    {"Trigger Shutdown", SHUTDOWN},
    { "Trigger Powerup", POWERUP},
    { "Trigger Sleep", POWERUP}
};



int gdo0 = 10;
int currentTrigger = -1;
TaskHandle_t Task1;
void taskFunction(void * parameter);

void print_wakeup_reason();


void app_main(void)
{

    print_wakeup_reason();
    Init();
    
    if(getCC1101()){
        ESP_LOGI(TAG, "Connection OK");
    }else{
        ESP_LOGI(TAG, "Connection Error");
    }

    setGDO0(gdo0);
    setCCMode(1);          // set config for internal transmission mode.
    setModulation(0);      // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
    setMHZ(433.92);        // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
    setDeviation(47.60);   // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
    setChannel(0);         // Set the Channelnumber from 0 to 255. Default is cahnnel 0.
    setChsp(199.95);       // The channel spacing is multiplied by the channel number CHAN and added to the base frequency in kHz. Value from 25.39 to 405.45. Default is 199.95 kHz.
    setRxBW(812.50);       // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
    setDRate(99.97);       // Set the Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
    setPA(10);             // Set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
    setSyncMode(2);        // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
    setSyncWord(ON_SYNC_WORD_BYTE, 145); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
    setAdrChk(0);          // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.
    setAddr(0);            // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
    setWhiteData(0);       // Turn data whitening on / off. 0 = Whitening off. 1 = Whitening on.
    setPktFormat(0);       // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
    setLengthConfig(1);    // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
    setPacketLength(0);    // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.
    setCrc(1);             // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
    setCRC_AF(0);          // Enable automatic flush of RX FIFO when CRC is not OK. This requires that only one packet is in the RXIFIFO and that packet length is limited to the RX FIFO size.
    setDcFilterOff(0);     // Disable digital DC blocking filter before demodulator. Only for data rates ≤ 250 kBaud The recommended IF frequency changes when the DC blocking is disabled. 1 = Disable (current optimized). 0 = Enable (better sensitivity).
    setManchester(0);      // Enables Manchester encoding/decoding. 0 = Disable. 1 = Enable.
    setFEC(0);             // Enable Forward Error Correction (FEC) with interleaving for packet payload (Only supported for fixed packet length mode. 0 = Disable. 1 = Enable.
    setPRE(0);             // Sets the minimum number of preamble bytes to be transmitted. Values: 0 : 2, 1 : 3, 2 : 4, 3 : 6, 4 : 8, 5 : 12, 6 : 16, 7 : 24
    setPQT(0);             // Preamble quality estimator threshold. The preamble quality estimator increases an internal counter by one each time a bit is received that is different from the previous bit, and decreases the counter by 8 each time a bit is received that is the same as the last bit. A threshold of 4∙PQT for this counter is used to gate sync word detection. When PQT=0 a sync word is always accepted.
    setAppendStatus(0);    // When enabled, two status bytes will be appended to the payload of the packet. The status bytes contain RSSI and LQI values, as well as CRC OK.

    
    ESP_LOGI(TAG, "Rx Mode");
    xTaskCreate(
        taskFunction,     // Function to be called
        "Task1",          // Name of the task
        10000,            // Stack size
        NULL,             // Parameter
        1,                // Priority (1 is the lowest priority on ESP32)
        &Task1            // Task handle
    );

    while(1){
        if (CheckReceiveFlag()){
                
            //CRC Check. If "setCrc(false)" crc returns always OK!
            if (CheckCRC()){ 

            //Rssi Level in dBm
                ESP_LOGI(TAG,"Rssi: %d", getRssi());

            //Link Quality Indicator
                ESP_LOGI(TAG,"LQI: %d", getLqi());

            //Get received Data and calculate length
                int len = ReceiveData(buffer);
                buffer[len] = '\0';

            //Print received in char format.
                ESP_LOGI(TAG," %s", (char *)buffer);

            //Print received in bytes format.
                for (int i = 0; i<len; i++){
                    ESP_LOGI(TAG," %d", buffer[i]);
                }

                char* packet = (char*) buffer;
                // if (!strcmp(packet, "Trigger Sleep"))
                // {
                //     setSyncWord(WAKE_UP_SYNC_WORD_BYTE, 145);
                //     esp_sleep_enable_ext0_wakeup(GPIO_NUM_10, 1);
                //     ESP_LOGI(TAG,"Going to sleep now");
                //     delay(1000);
                //     esp_deep_sleep_start();
                // }

                // if (!strcmp(packet, "Trigger Shutdown"))
                // {
                //     setSyncWord(SHUTDOWN_SYNC_WORD_BYTE, 145);
                //     ESP_LOGI(TAG,"ESP SHUTTING DOWN");
                // }
                
                // if (!strcmp(packet, "Trigger Powerup"))
                // {
                //     setSyncWord(ON_SYNC_WORD_BYTE, 145);
                //     ESP_LOGI(TAG,"ESP Powered Up");
                // }

                // Iterates through every trigger member and check which is currently applied
                for(int i = 0; i <= NUM_TRIGGERS; i++){
                    if(!strcmp(packet, obj[i].methodstr)){
                        currentTrigger = i;
                        break;
                    }         
                    currentTrigger = -1;           
                }


                switch (currentTrigger)
                {
                case SHUTDOWN:
                    setSyncWord(SHUTDOWN_SYNC_WORD_BYTE, 145);
                    ESP_LOGI(TAG,"ESP SHUTTING DOWN");
                    break;  
                case POWERUP:
                    setSyncWord(ON_SYNC_WORD_BYTE, 145);
                    ESP_LOGI(TAG,"ESP Powered Up");
                    break;   
                case SLEEP:
                    setSyncWord(WAKE_UP_SYNC_WORD_BYTE, 145);
                    esp_sleep_enable_ext0_wakeup(GPIO_NUM_10, 1);
                    ESP_LOGI(TAG,"Going to sleep now");
                    delay(1000);
                    esp_deep_sleep_start();
                    break;                
                default:
                    break;
                }

            }
        }

    }
}



void taskFunction(void * parameter) {
  (void) parameter;
  
  while(1) 
  {
    int pinState = digitalRead(10); // Read the state of pin 10
    if(pinState)
    {
      uint64_t start = micros();
      while(digitalRead(10));
      uint64_t duration = micros() - start;
      ESP_LOGI(TAG, "GDO0 held high for us :  %llu", duration);
    }
    vTaskDelay(1/ portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}


void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : ESP_LOGI(TAG, "Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : ESP_LOGI(TAG, "Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : ESP_LOGI(TAG, "Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : ESP_LOGI(TAG, "Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : ESP_LOGI(TAG, "Wakeup caused by ULP program"); break;
    default :ESP_LOGI(TAG, "Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}