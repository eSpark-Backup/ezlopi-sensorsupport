#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define ON_SYNC_WORD_BYTE 211
#define WAKE_UP_SYNC_WORD_BYTE 212
#define SHUTDOWN_SYNC_WORD_BYTE 213

void uart_recieve_upcall(uint8_t *buffer, s_ezlopi_uart_object_handle_t uart_object_handle, void *user_args);

int incommingByte;
static const char *TAG = "TRANSMITTER";

enum TRIGGER
{
    SHUTDOWN,
    POWERUP,
    SLEEP
};

int gdo0 = 10;

void print_wakeup_reason();

void app_main(void)
{

    Init();
    ezlopi_uart_init(115200, 43, 44, uart_recieve_upcall, NULL);
    if (getCC1101())
    {
        ESP_LOGI(TAG, "Connection OK");
    }
    else
    {
        ESP_LOGI(TAG, "Connection Error");
    }

    setGDO0(gdo0);
    setCCMode(1);                        // set config for internal transmission mode.
    setModulation(0);                    // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
    setMHZ(433.92);                      // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
    setDeviation(47.60);                 // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
    setChannel(0);                       // Set the Channelnumber from 0 to 255. Default is cahnnel 0.
    setChsp(199.95);                     // The channel spacing is multiplied by the channel number CHAN and added to the base frequency in kHz. Value from 25.39 to 405.45. Default is 199.95 kHz.
    setRxBW(812.50);                     // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
    setDRate(99.97);                     // Set the Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
    setPA(10);                           // Set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
    setSyncMode(2);                      // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
    setSyncWord(ON_SYNC_WORD_BYTE, 145); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
    setAdrChk(0);                        // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.
    setAddr(0);                          // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
    setWhiteData(0);                     // Turn data whitening on / off. 0 = Whitening off. 1 = Whitening on.
    setPktFormat(0);                     // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
    setLengthConfig(1);                  // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
    setPacketLength(0);                  // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.
    setCrc(1);                           // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
    setCRC_AF(0);                        // Enable automatic flush of RX FIFO when CRC is not OK. This requires that only one packet is in the RXIFIFO and that packet length is limited to the RX FIFO size.
    setDcFilterOff(0);                   // Disable digital DC blocking filter before demodulator. Only for data rates ≤ 250 kBaud The recommended IF frequency changes when the DC blocking is disabled. 1 = Disable (current optimized). 0 = Enable (better sensitivity).
    setManchester(0);                    // Enables Manchester encoding/decoding. 0 = Disable. 1 = Enable.
    setFEC(0);                           // Enable Forward Error Correction (FEC) with interleaving for packet payload (Only supported for fixed packet length mode. 0 = Disable. 1 = Enable.
    setPRE(0);                           // Sets the minimum number of preamble bytes to be transmitted. Values: 0 : 2, 1 : 3, 2 : 4, 3 : 6, 4 : 8, 5 : 12, 6 : 16, 7 : 24
    setPQT(0);                           // Preamble quality estimator threshold. The preamble quality estimator increases an internal counter by one each time a bit is received that is different from the previous bit, and decreases the counter by 8 each time a bit is received that is the same as the last bit. A threshold of 4∙PQT for this counter is used to gate sync word detection. When PQT=0 a sync word is always accepted.
    setAppendStatus(0);                  // When enabled, two status bytes will be appended to the payload of the packet. The status bytes contain RSSI and LQI values, as well as CRC OK.

    ESP_LOGI(TAG, "Tx Mode");
    ESP_LOGI(TAG, "Available options:");
    ESP_LOGI(TAG, "1: Trigger Sleep");
    ESP_LOGI(TAG, "2: Trigger Wakeup");
    ESP_LOGI(TAG, "3: Trigger Shutdown");
    ESP_LOGI(TAG, "4: Trigger Powerup");

    while (1)
    {
        if (incommingByte == 49)
        {
            setSyncWord(ON_SYNC_WORD_BYTE, 145);
            SendData3("Trigger Sleep", 100);
            ESP_LOGI(TAG, "Trigger Sleep\n");
        }
        if (incommingByte == 50)
        {
            setSyncWord(WAKE_UP_SYNC_WORD_BYTE, 145);
            SendData3("Trigger Wakeup", 100);
            ESP_LOGI(TAG, "Trigger Wakeup\n");
        }
        if (incommingByte == 51)
        {
            setSyncWord(ON_SYNC_WORD_BYTE, 145);
            SendData3("Trigger Shutdown", 100);
            ESP_LOGI(TAG, "Trigger Shutdown\n");
        }
        if (incommingByte == 52)
        {
            setSyncWord(SHUTDOWN_SYNC_WORD_BYTE, 145);
            SendData3("Trigger Powerup", 100);
            ESP_LOGI(TAG, "Trigger Powerup\n");
        }
        incommingByte = 0;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void uart_recieve_upcall(uint8_t *buffer, s_ezlopi_uart_object_handle_t uart_object_handle, void *user_args)
{
    incommingByte = (int)*buffer;
    ESP_LOGI(TAG, "Rcieved from UART %d", incommingByte);
}