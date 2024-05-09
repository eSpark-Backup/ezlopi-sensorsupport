/*
  ELECHOUSE_CC1101.cpp - CC1101 module library
  Copyright (c) 2010 Michael.
    Author: Michael, <www.elechouse.com>
    Version: November 12, 2010

  This library is designed to use CC1101/CC1100 module on Arduino platform.
  CC1101/CC1100 module is an useful wireless module.Using the functions of the 
  library, you can easily send and receive data by the CC1101/CC1100 module. 
  Just have fun!
  For the details, please refer to the datasheet of CC1100/CC1101.
----------------------------------------------------------------------------------------------------------------
cc1101 Driver for RC Switch. Mod by Little Satan. With permission to modify and publish Wilson Shen (ELECHOUSE).
----------------------------------------------------------------------------------------------------------------
*/
#ifndef ELECHOUSE_CC1101_SRC_DRV_h
#define ELECHOUSE_CC1101_SRC_DRV_h

#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "RCSwitch.h"
#include "ezlopi_hal_uart.h"
#include <driver/spi_master.h>
#include "ezlopi_hal_spi_master.h"

typedef uint8_t byte;
#define HIGH 1
#define LOW 0
// static const char* TAG = "CC1101";
//***************************************CC1101 define**************************************************//
// CC1101 CONFIG REGSITER
#define CC1101_IOCFG2       0x00        // GDO2 output pin configuration
#define CC1101_IOCFG1       0x01        // GDO1 output pin configuration
#define CC1101_IOCFG0       0x02        // GDO0 output pin configuration
#define CC1101_FIFOTHR      0x03        // RX FIFO and TX FIFO thresholds
#define CC1101_SYNC1        0x04        // Sync word, high INT8U
#define CC1101_SYNC0        0x05        // Sync word, low INT8U
#define CC1101_PKTLEN       0x06        // Packet length
#define CC1101_PKTCTRL1     0x07        // Packet automation control
#define CC1101_PKTCTRL0     0x08        // Packet automation control
#define CC1101_ADDR         0x09        // Device address
#define CC1101_CHANNR       0x0A        // Channel number
#define CC1101_FSCTRL1      0x0B        // Frequency synthesizer control
#define CC1101_FSCTRL0      0x0C        // Frequency synthesizer control
#define CC1101_FREQ2        0x0D        // Frequency control word, high INT8U
#define CC1101_FREQ1        0x0E        // Frequency control word, middle INT8U
#define CC1101_FREQ0        0x0F        // Frequency control word, low INT8U
#define CC1101_MDMCFG4      0x10        // Modem configuration
#define CC1101_MDMCFG3      0x11        // Modem configuration
#define CC1101_MDMCFG2      0x12        // Modem configuration
#define CC1101_MDMCFG1      0x13        // Modem configuration
#define CC1101_MDMCFG0      0x14        // Modem configuration
#define CC1101_DEVIATN      0x15        // Modem deviation setting
#define CC1101_MCSM2        0x16        // Main Radio Control State Machine configuration
#define CC1101_MCSM1        0x17        // Main Radio Control State Machine configuration
#define CC1101_MCSM0        0x18        // Main Radio Control State Machine configuration
#define CC1101_FOCCFG       0x19        // Frequency Offset Compensation configuration
#define CC1101_BSCFG        0x1A        // Bit Synchronization configuration
#define CC1101_AGCCTRL2     0x1B        // AGC control
#define CC1101_AGCCTRL1     0x1C        // AGC control
#define CC1101_AGCCTRL0     0x1D        // AGC control
#define CC1101_WOREVT1      0x1E        // High INT8U Event 0 timeout
#define CC1101_WOREVT0      0x1F        // Low INT8U Event 0 timeout
#define CC1101_WORCTRL      0x20        // Wake On Radio control
#define CC1101_FREND1       0x21        // Front end RX configuration
#define CC1101_FREND0       0x22        // Front end TX configuration
#define CC1101_FSCAL3       0x23        // Frequency synthesizer calibration
#define CC1101_FSCAL2       0x24        // Frequency synthesizer calibration
#define CC1101_FSCAL1       0x25        // Frequency synthesizer calibration
#define CC1101_FSCAL0       0x26        // Frequency synthesizer calibration
#define CC1101_RCCTRL1      0x27        // RC oscillator configuration
#define CC1101_RCCTRL0      0x28        // RC oscillator configuration
#define CC1101_FSTEST       0x29        // Frequency synthesizer calibration control
#define CC1101_PTEST        0x2A        // Production test
#define CC1101_AGCTEST      0x2B        // AGC test
#define CC1101_TEST2        0x2C        // Various test settings
#define CC1101_TEST1        0x2D        // Various test settings
#define CC1101_TEST0        0x2E        // Various test settings

//CC1101 Strobe commands
#define CC1101_SRES         0x30        // Reset chip.
#define CC1101_SFSTXON      0x31        // Enable and calibrate frequency synthesizer (if MCSM0.FS_AUTOCAL=1).
                                        // If in RX/TX: Go to a wait state where only the synthesizer is
                                        // running (for quick RX / TX turnaround).
#define CC1101_SXOFF        0x32        // Turn off crystal oscillator.
#define CC1101_SCAL         0x33        // Calibrate frequency synthesizer and turn it off
                                        // (enables quick start).
#define CC1101_SRX          0x34        // Enable RX. Perform calibration first if coming from IDLE and
                                        // MCSM0.FS_AUTOCAL=1.
#define CC1101_STX          0x35        // In IDLE state: Enable TX. Perform calibration first if
                                        // MCSM0.FS_AUTOCAL=1. If in RX state and CCA is enabled:
                                        // Only go to TX if channel is clear.
#define CC1101_SIDLE        0x36        // Exit RX / TX, turn off frequency synthesizer and exit
                                        // Wake-On-Radio mode if applicable.
#define CC1101_SAFC         0x37        // Perform AFC adjustment of the frequency synthesizer
#define CC1101_SWOR         0x38        // Start automatic RX polling sequence (Wake-on-Radio)
#define CC1101_SPWD         0x39        // Enter power down mode when CSn goes high.
#define CC1101_SFRX         0x3A        // Flush the RX FIFO buffer.
#define CC1101_SFTX         0x3B        // Flush the TX FIFO buffer.
#define CC1101_SWORRST      0x3C        // Reset real time clock.
#define CC1101_SNOP         0x3D        // No operation. May be used to pad strobe commands to two
                                        // INT8Us for simpler software.
//CC1101 STATUS REGSITER
#define CC1101_PARTNUM      0x30
#define CC1101_VERSION      0x31
#define CC1101_FREQEST      0x32
#define CC1101_LQI          0x33
#define CC1101_RSSI         0x34
#define CC1101_MARCSTATE    0x35
#define CC1101_WORTIME1     0x36
#define CC1101_WORTIME0     0x37
#define CC1101_PKTSTATUS    0x38
#define CC1101_VCO_VC_DAC   0x39
#define CC1101_TXBYTES      0x3A
#define CC1101_RXBYTES      0x3B

//CC1101 PATABLE,TXFIFO,RXFIFO
#define CC1101_PATABLE      0x3E
#define CC1101_TXFIFO       0x3F
#define CC1101_RXFIFO       0x3F

//************************************* class **************************************************//

#define delay(ms) vTaskDelay(ms/ portTICK_PERIOD_MS)

typedef enum events
{
    TRANSMITTING = 0,
    SET_BUTTON,
    USER_INPUT,
    FREQ_SWEEP
} events;

typedef struct cc1101_data
{
    float freq;
    events event;
    unsigned long code;
} cc1101_data;

typedef struct
{
    int gdo0;
    int gdo2;
    int shutdown;
} cc1101_gpios_t;

// class ELECHOUSE_CC1101
// {
// private:
  esp_err_t CC1101_SpiStart();
  void CC1101_SpiEnd(void);
  void CC1101_GDO_Set(void);
  void CC1101_GDO0_Set(void);
  void CC1101_Reset();
  void CC1101_setSpi(void);
  void CC1101_RegConfigSettings(void);
  void CC1101_Calibrate(void);
  void CC1101_Split_PKTCTRL0(void);
  void CC1101_Split_PKTCTRL1(void);
  void CC1101_Split_MDMCFG1(void);
  void CC1101_Split_MDMCFG2(void);
  void CC1101_Split_MDMCFG4(void);
// public:
  void CC1101_Init(s_ezlopi_spi_master_t *transReceive, cc1101_gpios_t *cc1101_gpios);
  byte CC1101_SpiReadStatus(byte addr);
  void CC1101_setSpiPin(s_ezlopi_spi_master_t *transReceive);
  void CC1101_addSpiPin(byte sck, byte miso, byte mosi, byte ss, byte modul);
  void CC1101_SpiConfig(s_ezlopi_spi_master_t *transReceive);
  void CC1101_PrintSpiConfig(s_ezlopi_spi_master_t *transReceive);
  void CC1101_setGDO(cc1101_gpios_t *cc1101_gpios);
  void CC1101_setGDO0(byte gdo0);
  void CC1101_addGDO(byte gdo0, byte gdo2, byte modul);
  void CC1101_addGDO0(byte gdo0, byte modul);
  void CC1101_setModul(byte modul);
  void CC1101_setCCMode(bool s);
  void CC1101_setModulation(byte m);
  void CC1101_setPA(int p);
  void CC1101_setMHZ(float mhz);
  void CC1101_setChannel(byte chnl);
  void CC1101_setChsp(float f);
  void CC1101_setRxBW(float f);
  void CC1101_setDRate(float d);
  void CC1101_setDeviation(float d);
  void CC1101_SetTx(void);
  void CC1101_SetRx(void);
  // void SetTx(float mhz);
  // void SetRx(float mhz);
  int CC1101_getRssi(void);
  byte CC1101_getLqi(void);
  void CC1101_setSres(void);
  void CC1101_setSidle(void);
  void CC1101_goSleep(void);
  void CC1101_SendData(byte *txBuffer, byte size);
  void CC1101_SendData1(char *txchar);
  void CC1101_SendData2(byte *txBuffer, byte size, int t);
  void CC1101_SendData3(char *txchar, int t);
  byte CC1101_CheckReceiveFlag(void);
  byte CC1101_ReceiveData(byte *rxBuffer);
  bool CC1101_CheckCRC(void);
  void CC1101_SpiStrobe(byte strobe);
  void CC1101_SpiWriteReg(byte addr, byte value);
  void CC1101_SpiWriteBurstReg(byte addr, byte *buffer, byte num);
  byte CC1101_SpiReadReg(byte addr);
  void CC1101_SpiReadBurstReg(byte addr, byte *buffer, byte num);
  void CC1101_setClb(byte b, byte s, byte e);
  bool CC1101_getCC1101(void);
  byte CC1101_getMode(void);
  void CC1101_setSyncWord(byte sh, byte sl);
  void CC1101_setAddr(byte v);
  void CC1101_setWhiteData(bool v);
  void CC1101_setPktFormat(byte v);
  void CC1101_setCrc(bool v);
  void CC1101_setLengthConfig(byte v);
  void CC1101_setPacketLength(byte v);
  void CC1101_setDcFilterOff(bool v);
  void CC1101_setManchester(bool v);
  void CC1101_setSyncMode(byte v);
  void CC1101_setFEC(bool v);
  void CC1101_setPRE(byte v);
  void CC1101_setPQT(byte v);
  void CC1101_setCRC_AF(bool v);
  void CC1101_setAppendStatus(bool v);
  void CC1101_setAdrChk(byte v);
  bool CC1101_CheckRxFifo(int t);
  byte CC1101_getPartnum(void);
  byte CC1101_getVersion(void);
  void CC1101_spiTransferWord(uint16_t );
  float CC1101_sweepFrequency();
  void CC1101_API_function(void* );
  // esp_err_t getData(receivedData_t* );
  // esp_err_t setup();
  void CC1101_Transmitting(unsigned long);
// };
float CC1101_SearchFrequency(float mhz, float freqSpace, bool isDetails);
float CC1101_searchFrequencyForRange(float startMhz, float endMhz, float freqSpace, bool isDetails);
esp_err_t CC1101_spi_init();
uint8_t CC1101_spi_transfer(uint8_t);
void CC1101_ezlopi_cc1101_upcall(uint8_t* , s_ezlopi_uart_object_handle_t , void* );
void cc1101_read_upcall(uint8_t* ,cc1101_data* );
void CC1101_structToJSON(cc1101_data*);
// extern ELECHOUSE ELECHOUSE_cc1101;

#endif
