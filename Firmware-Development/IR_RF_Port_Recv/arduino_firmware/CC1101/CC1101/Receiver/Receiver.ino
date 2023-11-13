// These examples are from the Electronics Cookbook by Simon Monk
//https://github.com/LSatan/SmartRC-CC1101-Driver-Lib
// mod by Little_S@tan
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define ON_SYNC_WORD_BYTE 211
#define WAKE_UP_SYNC_WORD_BYTE 212
#define SHUTDOWN_SYNC_WORD_BYTE 213

int gdo0 = 10;
TaskHandle_t Task1;
void taskFunction(void * parameter);

void print_wakeup_reason();

void setup(){
  
    Serial.begin(115200);
    print_wakeup_reason();
    ELECHOUSE_cc1101.setSpiPin(13, 11, 12, 14); // Set custom pins: (SCK, MISO, MOSI, CSN). Must be BEFORE Init()!
    if (ELECHOUSE_cc1101.getCC1101()){      // Check the CC1101 Spi connection.
    Serial.println("Connection OK");
    }else{
    Serial.println("Connection Error");
    }

    ELECHOUSE_cc1101.Init();                // must be set to initialize the cc1101!
    ELECHOUSE_cc1101.setGDO0(gdo0);         // set lib internal gdo pin (gdo0). Gdo2 not use for this example.
    ELECHOUSE_cc1101.setCCMode(1);          // set config for internal transmission mode.
    ELECHOUSE_cc1101.setModulation(0);      // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
    ELECHOUSE_cc1101.setMHZ(433.92);        // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
    ELECHOUSE_cc1101.setDeviation(47.60);   // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
    ELECHOUSE_cc1101.setChannel(0);         // Set the Channelnumber from 0 to 255. Default is cahnnel 0.
    ELECHOUSE_cc1101.setChsp(199.95);       // The channel spacing is multiplied by the channel number CHAN and added to the base frequency in kHz. Value from 25.39 to 405.45. Default is 199.95 kHz.
    ELECHOUSE_cc1101.setRxBW(812.50);       // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
    ELECHOUSE_cc1101.setDRate(99.97);       // Set the Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
    ELECHOUSE_cc1101.setPA(10);             // Set TxPower. The following settings are possible depending on the frequency band.  (-30  -20  -15  -10  -6    0    5    7    10   11   12) Default is max!
    ELECHOUSE_cc1101.setSyncMode(2);        // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
    ELECHOUSE_cc1101.setSyncWord(ON_SYNC_WORD_BYTE, 145); // Set sync word. Must be the same for the transmitter and receiver. (Syncword high, Syncword low)
    ELECHOUSE_cc1101.setAdrChk(0);          // Controls address check configuration of received packages. 0 = No address check. 1 = Address check, no broadcast. 2 = Address check and 0 (0x00) broadcast. 3 = Address check and 0 (0x00) and 255 (0xFF) broadcast.
    ELECHOUSE_cc1101.setAddr(0);            // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
    ELECHOUSE_cc1101.setWhiteData(0);       // Turn data whitening on / off. 0 = Whitening off. 1 = Whitening on.
    ELECHOUSE_cc1101.setPktFormat(0);       // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
    ELECHOUSE_cc1101.setLengthConfig(1);    // 0 = Fixed packet length mode. 1 = Variable packet length mode. 2 = Infinite packet length mode. 3 = Reserved
    ELECHOUSE_cc1101.setPacketLength(0);    // Indicates the packet length when fixed packet length mode is enabled. If variable packet length mode is used, this value indicates the maximum packet length allowed.
    ELECHOUSE_cc1101.setCrc(1);             // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.
    ELECHOUSE_cc1101.setCRC_AF(0);          // Enable automatic flush of RX FIFO when CRC is not OK. This requires that only one packet is in the RXIFIFO and that packet length is limited to the RX FIFO size.
    ELECHOUSE_cc1101.setDcFilterOff(0);     // Disable digital DC blocking filter before demodulator. Only for data rates ≤ 250 kBaud The recommended IF frequency changes when the DC blocking is disabled. 1 = Disable (current optimized). 0 = Enable (better sensitivity).
    ELECHOUSE_cc1101.setManchester(0);      // Enables Manchester encoding/decoding. 0 = Disable. 1 = Enable.
    ELECHOUSE_cc1101.setFEC(0);             // Enable Forward Error Correction (FEC) with interleaving for packet payload (Only supported for fixed packet length mode. 0 = Disable. 1 = Enable.
    ELECHOUSE_cc1101.setPRE(0);             // Sets the minimum number of preamble bytes to be transmitted. Values: 0 : 2, 1 : 3, 2 : 4, 3 : 6, 4 : 8, 5 : 12, 6 : 16, 7 : 24
    ELECHOUSE_cc1101.setPQT(0);             // Preamble quality estimator threshold. The preamble quality estimator increases an internal counter by one each time a bit is received that is different from the previous bit, and decreases the counter by 8 each time a bit is received that is the same as the last bit. A threshold of 4∙PQT for this counter is used to gate sync word detection. When PQT=0 a sync word is always accepted.
    ELECHOUSE_cc1101.setAppendStatus(0);    // When enabled, two status bytes will be appended to the payload of the packet. The status bytes contain RSSI and LQI values, as well as CRC OK.

    Serial.println("Rx Mode");
    xTaskCreate(
    taskFunction,     // Function to be called
    "Task1",          // Name of the task
    10000,            // Stack size
    NULL,             // Parameter
    1,                // Priority (1 is the lowest priority on ESP32)
    &Task1            // Task handle
  );
}

byte buffer[61] = {0};

void loop(){

   //Checks whether something has been received.
  if (ELECHOUSE_cc1101.CheckReceiveFlag()){
    
   //CRC Check. If "setCrc(false)" crc returns always OK!
  if (ELECHOUSE_cc1101.CheckCRC()){ 

   //Rssi Level in dBm
    Serial.print("Rssi: ");
    Serial.println(ELECHOUSE_cc1101.getRssi());

   //Link Quality Indicator
    Serial.print("LQI: ");
    Serial.println(ELECHOUSE_cc1101.getLqi());

   //Get received Data and calculate length
    int len = ELECHOUSE_cc1101.ReceiveData(buffer);
    buffer[len] = '\0';

   //Print received in char format.
    Serial.println((char *) buffer);

   //Print received in bytes format.
    for (int i = 0; i<len; i++){
    Serial.print(buffer[i]);
    Serial.print(",");
    }
    Serial.println();

    char* packet = (char*) buffer;
    if (!strcmp(packet, "Trigger Sleep"))
    {
      ELECHOUSE_cc1101.setSyncWord(WAKE_UP_SYNC_WORD_BYTE, 145);
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_10, 1);
      Serial.println("Going to sleep now");
      delay(1000);
      esp_deep_sleep_start();
    }

    if (!strcmp(packet, "Trigger Shutdown"))
    {
      ELECHOUSE_cc1101.setSyncWord(SHUTDOWN_SYNC_WORD_BYTE, 145);
      Serial.println("ESP SHUTTING DOWN");
    }
    
    if (!strcmp(packet, "Trigger Powerup"))
    {
      ELECHOUSE_cc1101.setSyncWord(ON_SYNC_WORD_BYTE, 145);
      Serial.println("ESP Powered Up");
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
      Serial.print("GDO0 held high for us :  ");
      Serial.println(duration);
    }
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
