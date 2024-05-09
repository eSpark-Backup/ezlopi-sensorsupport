#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include <math.h>
#include "cJSON.h"
#include <string.h>
#include <string.h>
#include "esp_log.h"
#include "RCSwitch.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "ezlopi_hal_uart.h"
#include "driver/spi_slave.h"
#include "freertos/FreeRTOS.h"
#include <driver/spi_master.h>
#include "ezlopi_hal_spi_master.h"
/****************************************************************/
#define WRITE_BURST 0x40     // write burst
#define READ_SINGLE 0x80     // read single
#define READ_BURST 0xC0      // read burst
#define BYTES_IN_RXFIFO 0x7F // byte number in RXfifo
#define max_modul 6
#define OUTPUT GPIO_MODE_OUTPUT
#define INPUT GPIO_MODE_INPUT

#define HOST_ID SPI2_HOST

// static spi_device_handle_t _handle;

// RCSwitch mySwitch = RCSwitch();

/**
 * Arduino Macros
 */
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))
#define delayMicroseconds(us) esp_rom_delay_us(us)
#define LOW 0
#define HIGH 1
#define byte uint8_t

/**
 * PATABLE
 */
// const byte paTable[8] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60};

byte modulation = 2;
byte frend0;
byte chan = 0;
int pa = 12;
byte last_pa;
byte SCK_PIN;
byte MISO_PIN;
byte MOSI_PIN;
byte SS_PIN;
byte GDO0;
byte GDO2;
byte SCK_PIN_M[max_modul];
byte MISO_PIN_M[max_modul];
byte MOSI_PIN_M[max_modul];
byte SS_PIN_M[max_modul];
byte GDO0_M[max_modul];
byte GDO2_M[max_modul];
byte gdo_set = 0;
bool spi = 0;
bool ccmode = 0;
float MHz = 400;
byte m4RxBw = 0;
byte m4DaRa;
byte m2DCOFF;
byte m2MODFM;
byte m2MANCH;
byte m2SYNCM;
byte m1FEC;
byte m1PRE;
byte m1CHSP;
byte pc1PQT;
byte pc1CRC_AF;
byte pc1APP_ST;
byte pc1ADRCHK;
byte pc0WDATA;
byte pc0PktForm;
byte pc0CRC_EN;
byte pc0LenConf;
byte trxstate = 0;
byte clb1[2] = {24, 28};
byte clb2[2] = {31, 38};
byte clb3[2] = {65, 76};
byte clb4[2] = {77, 79};
static const char *TAG = "CC1101";
esp_err_t ret = ESP_FAIL;

s_ezlopi_spi_master_t *transReceive_t = NULL;
cc1101_gpios_t *CC1101_gpios = NULL;

Variables var = {
    .nReceivedValue = 0,
    .nReceivedValue = 0,
    .nReceivedBitlength = 0,
    .nReceivedDelay = 0,
    .nReceivedProtocol = 0,
    .nReceiveTolerance = 60,
    .nSeparationLimit = 4300,
};
cc1101_data data;
int track = 0;
bool task = true;
bool wFlag = true;
float freq_set;

static inline void cc1101_Select()
{
  digitalWrite(SS_PIN, LOW);
}
static inline void cc1101_Deselect()
{
  digitalWrite(SS_PIN, HIGH);
}
/**
 * Macros
 */
#define wait_Miso() while (gpio_get_level((gpio_num_t)MISO_PIN) > 0)
// Get GDO0 pin state
#define getGDO0state() gpio_get_level((gpio_num_t)GDO0)
// Wait until GDO0 line goes high
#define wait_GDO0_high() while (!getGDO0state())
// Wait until GDO0 line goes low
#define wait_GDO0_low() while (getGDO0state())

/****************************************************************/
uint8_t PA_TABLE[8] = {0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//                       -30  -20  -15  -10   0    5    7    10
uint8_t PA_TABLE_315[8] = {
    0x12,
    0x0D,
    0x1C,
    0x34,
    0x51,
    0x85,
    0xCB,
    0xC2,
}; // 300 - 348
uint8_t PA_TABLE_433[8] = {
    0x12,
    0x0E,
    0x1D,
    0x34,
    0x60,
    0x84,
    0xC8,
    0xC0,
}; // 387 - 464
//                        -30  -20  -15  -10  -6    0    5    7    10   12
uint8_t PA_TABLE_868[10] = {
    0x03,
    0x17,
    0x1D,
    0x26,
    0x37,
    0x50,
    0x86,
    0xCD,
    0xC5,
    0xC0,
}; // 779 - 899.99
//                        -30  -20  -15  -10  -6    0    5    7    10   11
uint8_t PA_TABLE_915[10] = {
    0x03,
    0x0E,
    0x1E,
    0x27,
    0x38,
    0x8E,
    0x84,
    0xCC,
    0xC3,
    0xC0,
}; // 900 - 928
/****************************************************************
 *FUNCTION NAME:SpiStart
 *FUNCTION     :spi communication start
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
esp_err_t CC1101_SpiStart()
{
  gpio_reset_pin((gpio_num_t)SS_PIN);
  gpio_set_direction((gpio_num_t)SS_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)SS_PIN, 1);
  return ezlopi_spi_master_init(transReceive_t);
}

/****************************************************************
 *FUNCTION NAME: GDO_Set()
 *FUNCTION     : set GDO0,GDO2 pin for serial pinmode.
 *INPUT        : none
 *OUTPUT       : none
 ****************************************************************/
void CC1101_GDO_Set(void)
{
  pinMode(GDO0, OUTPUT);
  pinMode(GDO2, INPUT);
}
/****************************************************************
 *FUNCTION NAME: GDO_Set()
 *FUNCTION     : set GDO0 for internal transmission mode.
 *INPUT        : none
 *OUTPUT       : none
 ****************************************************************/
void CC1101_GDO0_Set(void)
{
  pinMode(GDO0, INPUT);
}
/****************************************************************
 *FUNCTION NAME:Reset
 *FUNCTION     :CC1101 reset //details refer datasheet of CC1101/CC1100//
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_Reset()
{
  digitalWrite(SS_PIN, LOW);
  delay(1);
  digitalWrite(SS_PIN, HIGH);
  delay(1);
  digitalWrite(SS_PIN, LOW);
  while (digitalRead(MISO_PIN));
  CC1101_spi_transfer(CC1101_SRES);
  while (digitalRead(MISO_PIN));
  digitalWrite(SS_PIN, HIGH);
}

void CC1101_spiTransferWord(uint16_t inputData)
{

  CC1101_spi_transfer(inputData);
}
/****************************************************************
 *FUNCTION NAME:Init
 *FUNCTION     :CC1101 initialization
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/

void  CC1101_Init(s_ezlopi_spi_master_t *transReceive, cc1101_gpios_t *cc1101_gpios)
{
  CC1101_SpiConfig(transReceive);
  transReceive_t = transReceive;
  CC1101_gpios = cc1101_gpios;
  CC1101_PrintSpiConfig(transReceive_t);
  RCSwitchInit(&var);
  // enableTransmit(CC1101_gpios->gdo0, &var);                    // Receiver on interrupt 0
  CC1101_setSpiPin(transReceive_t);          // Set custom pins: (SCK, MISO, MOSI, CSN). Must be BEFORE Init()!
  CC1101_setGDO(CC1101_gpios);                      // Set custom GDO pins: (GDO0, GDO2). Gdo0 (tx) and Gdo2 (rx). Must be BEFORE Init();
  esp_err_t retu = CC1101_SpiStart();        // spi initialization
  ESP_RETURN_ON_ERROR(retu, TAG, "failed to initialize SPI");
  digitalWrite(SS_PIN, HIGH);
  digitalWrite(SCK_PIN, HIGH);
  digitalWrite(MOSI_PIN, LOW);
  CC1101_Reset();                            // CC1101 reset
  CC1101_RegConfigSettings();                // CC1101 register config
  enableReceive(CC1101_gpios->gdo2, &var);                    // Receiver on interrupt 0
}

void CC1101_SpiConfig(s_ezlopi_spi_master_t *transReceive)
{
  transReceive->enable = true;
  transReceive->mode = EZLOPI_SPI_CPOL_LOW_CPHA_LOW;
  transReceive->clock_speed_mhz = 5;
  transReceive->addr_bits = 0;
  transReceive->command_bits = 0;
  transReceive->transfer_sz = 32;
  transReceive->queue_size = 7;
  transReceive->flags = SPICOMMON_BUSFLAG_MASTER;
  transReceive->channel = EZLOPI_SPI_2;
}

void CC1101_PrintSpiConfig(s_ezlopi_spi_master_t *transReceive)
{
  ESP_LOGI(TAG, "Enable: %d",transReceive->enable);
  ESP_LOGI(TAG, "MISO: %d",transReceive->miso);
  ESP_LOGI(TAG, "MOSI: %d",transReceive->mosi);
  ESP_LOGI(TAG, "SCK: %d",transReceive->sck);
  ESP_LOGI(TAG, "SS: %d",transReceive->cs);
  ESP_LOGI(TAG, "MODE: %d",transReceive->mode);
  ESP_LOGI(TAG, "Clock Speed: %d",transReceive->clock_speed_mhz);
  ESP_LOGI(TAG, "Address Bits: %d",transReceive->addr_bits);
  ESP_LOGI(TAG, "Command Bits: %d",transReceive->command_bits);
  ESP_LOGI(TAG, "Transfer sz: %d",transReceive->transfer_sz);
  ESP_LOGI(TAG, "Queue: %d",transReceive->queue_size);
  ESP_LOGI(TAG, "Flags: %d",transReceive->flags);
  ESP_LOGI(TAG, "Channel: %d",transReceive->channel);
}
// /****************************************************************
// *FUNCTION NAME:CC1101_SpiWriteReg
// *FUNCTION     :CC1101 write data to register
// *INPUT        :addr: register address; value: register value
// *OUTPUT       :none
// ****************************************************************/
void CC1101_SpiWriteReg(byte addr, byte value)
{
  cc1101_Select();     // Select CC1101
  wait_Miso();         // Wait until MISO goes low
  CC1101_spi_transfer(addr);  // Send register address
  CC1101_spi_transfer(value); // Send value
  cc1101_Deselect();   // Deselect CC1101
}
/****************************************************************
 *FUNCTION NAME:CC1101_SpiWriteBurstReg
 *FUNCTION     :CC1101 write burst data to register
 *INPUT        :addr: register address; buffer:register value array; num:number to write
 *OUTPUT       :none
 ****************************************************************/
void CC1101_SpiWriteBurstReg(byte addr, byte *buffer, byte num)
{

  byte i, temp;
  temp = addr | WRITE_BURST;
  digitalWrite(SS_PIN, LOW);
  while (digitalRead(MISO_PIN))
    ;
  CC1101_spi_transfer(temp);
  for (i = 0; i < num; i++)
  {
    CC1101_spi_transfer(buffer[i]);
  }
  digitalWrite(SS_PIN, HIGH);
}
/****************************************************************
 *FUNCTION NAME:CC1101_SpiStrobe
 *FUNCTION     :CC1101 Strobe
 *INPUT        :strobe: command; //refer define in CC1101.h//
 *OUTPUT       :none
 ****************************************************************/
void CC1101_SpiStrobe(byte strobe)
{
  digitalWrite(SS_PIN, LOW);
  while (digitalRead(MISO_PIN))
    ;
  CC1101_spi_transfer(strobe);
  digitalWrite(SS_PIN, HIGH);
}
/****************************************************************
 *FUNCTION NAME:CC1101_SpiReadReg
 *FUNCTION     :CC1101 read data from register
 *INPUT        :addr: register address
 *OUTPUT       :register value
 ****************************************************************/
byte CC1101_SpiReadReg(byte addr)
{

  byte temp, val;
  temp = addr | READ_SINGLE;
  cc1101_Select();          // Select CC1101
  wait_Miso();              // Wait until MISO goes low
  CC1101_spi_transfer(temp);       // Send register address
  val = CC1101_spi_transfer(0x00); // Read result
  cc1101_Deselect();        // Deselect CC1101
  return val;
}

/****************************************************************
 *FUNCTION NAME:CC1101_SpiReadBurstReg
 *FUNCTION     :CC1101 read burst data from register
 *INPUT        :addr: register address; buffer:array to store register value; num: number to read
 *OUTPUT       :none
 ****************************************************************/
void CC1101_SpiReadBurstReg(byte addr, byte *buffer, byte num)
{

  byte i, temp;
  temp = addr | READ_BURST;
  digitalWrite(SS_PIN, LOW);
  while (digitalRead(MISO_PIN))
    ;
  CC1101_spi_transfer(temp);
  for (i = 0; i < num; i++)
  {
    buffer[i] = CC1101_spi_transfer(0);
  }
  digitalWrite(SS_PIN, HIGH);
}

/****************************************************************
 *FUNCTION NAME:CC1101_SpiReadStatus
 *FUNCTION     :CC1101 read status register
 *INPUT        :addr: register address
 *OUTPUT       :status value
 ****************************************************************/
byte CC1101_SpiReadStatus(byte addr)
{
  byte value = 0, temp;
  temp = addr | READ_BURST;
  digitalWrite(SS_PIN, LOW);
  while (digitalRead(MISO_PIN))
    ;
  // commented
  CC1101_spi_transfer(temp);
  value = CC1101_spi_transfer(0);
  digitalWrite(SS_PIN, HIGH);
  return value;
}
/****************************************************************
 *FUNCTION NAME:SPI pin Settings
 *FUNCTION     :Set Spi pins
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setSpi(void)
{
  SCK_PIN = 13;
  MISO_PIN = 11;
  MOSI_PIN = 12;
  SS_PIN = 14;
}

/****************************************************************
 *FUNCTION NAME:COSTUM SPI
 *FUNCTION     :set costum spi pins.
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setSpiPin(s_ezlopi_spi_master_t *transReceive)
{
  spi = 1;
  SCK_PIN = transReceive->sck;
  MISO_PIN = transReceive->miso;
  MOSI_PIN = transReceive->mosi;
  SS_PIN = transReceive->cs;
  transReceive->cs = -1;
}
/****************************************************************
 *FUNCTION NAME:COSTUM SPI
 *FUNCTION     :set costum spi pins.
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_addSpiPin(byte sck, byte miso, byte mosi, byte ss, byte modul)
{
  spi = 1;
  SCK_PIN_M[modul] = sck;
  MISO_PIN_M[modul] = miso;
  MOSI_PIN_M[modul] = mosi;
  SS_PIN_M[modul] = ss;
}
/****************************************************************
 *FUNCTION NAME:GDO Pin settings
 *FUNCTION     :set GDO Pins
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setGDO(cc1101_gpios_t *cc1101_gpios)
{
  GDO0 = cc1101_gpios->gdo0;
  GDO2 = cc1101_gpios->gdo2;
  CC1101_GDO_Set();
}
/****************************************************************
 *FUNCTION NAME:GDO0 Pin setting
 *FUNCTION     :set GDO0 Pin
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setGDO0(byte gdo0)
{
  GDO0 = gdo0;
  CC1101_GDO0_Set();
}
/****************************************************************
 *FUNCTION NAME:GDO Pin settings
 *FUNCTION     :add GDO Pins
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_addGDO(byte gdo0, byte gdo2, byte modul)
{
  GDO0_M[modul] = gdo0;
  GDO2_M[modul] = gdo2;
  gdo_set = 2;
  CC1101_GDO_Set();
}
/****************************************************************
 *FUNCTION NAME:add GDO0 Pin
 *FUNCTION     :add GDO0 Pin
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_addGDO0(byte gdo0, byte modul)
{
  GDO0_M[modul] = gdo0;
  gdo_set = 1;
  CC1101_GDO0_Set();
}
/****************************************************************
 *FUNCTION NAME:set Modul
 *FUNCTION     :change modul
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setModul(byte modul)
{
  SCK_PIN = SCK_PIN_M[modul];
  MISO_PIN = MISO_PIN_M[modul];
  MOSI_PIN = MOSI_PIN_M[modul];
  SS_PIN = SS_PIN_M[modul];
  if (gdo_set == 1)
  {
    GDO0 = GDO0_M[modul];
  }
  else if (gdo_set == 2)
  {
    GDO0 = GDO0_M[modul];
    GDO2 = GDO2_M[modul];
  }
}
/****************************************************************
 *FUNCTION NAME:CCMode
 *FUNCTION     :Format of RX and TX data
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setCCMode(bool s)
{
  ccmode = s;
  if (ccmode == 1)
  {
    CC1101_SpiWriteReg(CC1101_IOCFG2, 0x0B);
    CC1101_SpiWriteReg(CC1101_IOCFG0, 0x06);
    CC1101_SpiWriteReg(CC1101_PKTCTRL0, 0x05);
    CC1101_SpiWriteReg(CC1101_MDMCFG3, 0xF8);
    CC1101_SpiWriteReg(CC1101_MDMCFG4, 11 + m4RxBw);
  }
  else
  {
    CC1101_SpiWriteReg(CC1101_IOCFG2, 0x0D);
    CC1101_SpiWriteReg(CC1101_IOCFG0, 0x0D);
    CC1101_SpiWriteReg(CC1101_PKTCTRL0, 0x32);
    CC1101_SpiWriteReg(CC1101_MDMCFG3, 0x93);
    CC1101_SpiWriteReg(CC1101_MDMCFG4, 5 + m4RxBw);
  }
  CC1101_setModulation(modulation);
}
/****************************************************************
 *FUNCTION NAME:Modulation
 *FUNCTION     :set CC1101 Modulation
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setModulation(byte m)
{
  if (m > 4)
  {
    m = 4;
  }
  modulation = m;
  CC1101_Split_MDMCFG2();
  switch (m)
  {
  case 0:
    m2MODFM = 0x00;
    frend0 = 0x10;
    break; // 2-FSK
  case 1:
    m2MODFM = 0x10;
    frend0 = 0x10;
    break; // GFSK
  case 2:
    m2MODFM = 0x30;
    frend0 = 0x11;
    break; // ASK
  case 3:
    m2MODFM = 0x40;
    frend0 = 0x10;
    break; // 4-FSK
  case 4:
    m2MODFM = 0x70;
    frend0 = 0x10;
    break; // MSK
  }
  CC1101_SpiWriteReg(CC1101_MDMCFG2, m2DCOFF + m2MODFM + m2MANCH + m2SYNCM);
  CC1101_SpiWriteReg(CC1101_FREND0, frend0);
  CC1101_setPA(pa);
}
/****************************************************************
 *FUNCTION NAME:PA Power
 *FUNCTION     :set CC1101 PA Power
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setPA(int p)
{
  int a = 0;
  pa = p;

  if (MHz >= 300 && MHz <= 348)
  {
    if (pa <= -30)
    {
      a = PA_TABLE_315[0];
    }
    else if (pa > -30 && pa <= -20)
    {
      a = PA_TABLE_315[1];
    }
    else if (pa > -20 && pa <= -15)
    {
      a = PA_TABLE_315[2];
    }
    else if (pa > -15 && pa <= -10)
    {
      a = PA_TABLE_315[3];
    }
    else if (pa > -10 && pa <= 0)
    {
      a = PA_TABLE_315[4];
    }
    else if (pa > 0 && pa <= 5)
    {
      a = PA_TABLE_315[5];
    }
    else if (pa > 5 && pa <= 7)
    {
      a = PA_TABLE_315[6];
    }
    else if (pa > 7)
    {
      a = PA_TABLE_315[7];
    }
    last_pa = 1;
  }
  else if (MHz >= 378 && MHz <= 464)
  {
    if (pa <= -30)
    {
      a = PA_TABLE_433[0];
    }
    else if (pa > -30 && pa <= -20)
    {
      a = PA_TABLE_433[1];
    }
    else if (pa > -20 && pa <= -15)
    {
      a = PA_TABLE_433[2];
    }
    else if (pa > -15 && pa <= -10)
    {
      a = PA_TABLE_433[3];
    }
    else if (pa > -10 && pa <= 0)
    {
      a = PA_TABLE_433[4];
    }
    else if (pa > 0 && pa <= 5)
    {
      a = PA_TABLE_433[5];
    }
    else if (pa > 5 && pa <= 7)
    {
      a = PA_TABLE_433[6];
    }
    else if (pa > 7)
    {
      a = PA_TABLE_433[7];
    }
    last_pa = 2;
  }
  else if (MHz >= 779 && MHz <= 899.99)
  {
    if (pa <= -30)
    {
      a = PA_TABLE_868[0];
    }
    else if (pa > -30 && pa <= -20)
    {
      a = PA_TABLE_868[1];
    }
    else if (pa > -20 && pa <= -15)
    {
      a = PA_TABLE_868[2];
    }
    else if (pa > -15 && pa <= -10)
    {
      a = PA_TABLE_868[3];
    }
    else if (pa > -10 && pa <= -6)
    {
      a = PA_TABLE_868[4];
    }
    else if (pa > -6 && pa <= 0)
    {
      a = PA_TABLE_868[5];
    }
    else if (pa > 0 && pa <= 5)
    {
      a = PA_TABLE_868[6];
    }
    else if (pa > 5 && pa <= 7)
    {
      a = PA_TABLE_868[7];
    }
    else if (pa > 7 && pa <= 10)
    {
      a = PA_TABLE_868[8];
    }
    else if (pa > 10)
    {
      a = PA_TABLE_868[9];
    }
    last_pa = 3;
  }
  else if (MHz >= 900 && MHz <= 928)
  {
    if (pa <= -30)
    {
      a = PA_TABLE_915[0];
    }
    else if (pa > -30 && pa <= -20)
    {
      a = PA_TABLE_915[1];
    }
    else if (pa > -20 && pa <= -15)
    {
      a = PA_TABLE_915[2];
    }
    else if (pa > -15 && pa <= -10)
    {
      a = PA_TABLE_915[3];
    }
    else if (pa > -10 && pa <= -6)
    {
      a = PA_TABLE_915[4];
    }
    else if (pa > -6 && pa <= 0)
    {
      a = PA_TABLE_915[5];
    }
    else if (pa > 0 && pa <= 5)
    {
      a = PA_TABLE_915[6];
    }
    else if (pa > 5 && pa <= 7)
    {
      a = PA_TABLE_915[7];
    }
    else if (pa > 7 && pa <= 10)
    {
      a = PA_TABLE_915[8];
    }
    else if (pa > 10)
    {
      a = PA_TABLE_915[9];
    }
    last_pa = 4;
  }
  if (modulation == 2)
  {
    PA_TABLE[0] = 0;
    PA_TABLE[1] = a;
  }
  else
  {
    PA_TABLE[0] = a;
    PA_TABLE[1] = 0;
  }
  CC1101_SpiWriteBurstReg(CC1101_PATABLE, PA_TABLE, 8);
}
/****************************************************************
 *FUNCTION NAME:Frequency Calculator
 *FUNCTION     :Calculate the basic frequency.
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setMHZ(float mhz)
{
  uint16_t freq2 = 0;
  uint16_t freq1 = 0;
  uint16_t freq0 = 0;
  // MHz = mhz;

  for (bool i = 0; i == 0;)
  {
    if (mhz >= 26)
    {
      mhz -= 26;
      freq2 += 1;
    }
    else if (mhz >= 0.1015625)
    {
      mhz -= 0.1015625;
      freq1 += 1;
    }
    else if (mhz >= 0.00039675)
    {
      mhz -= 0.00039675;
      freq0 += 1;
    }
    else
    {
      i = 1;
    }
  }
  if (freq0 > 255)
  {
    freq1 += 1;
    freq0 -= 256;
  }

  CC1101_SpiWriteReg(CC1101_FREQ2, freq2);
  CC1101_SpiWriteReg(CC1101_FREQ1, freq1);
  CC1101_SpiWriteReg(CC1101_FREQ0, freq0);

  CC1101_Calibrate();
}
/****************************************************************
 *FUNCTION NAME:CC1101_Calibrate
 *FUNCTION     :CC1101_Calibrate frequency
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_Calibrate(void)
{

  if (MHz >= 300 && MHz <= 348)
  {
    CC1101_SpiWriteReg(CC1101_FSCTRL0, map(MHz, 300, 348, clb1[0], clb1[1]));
    if (MHz < 322.88)
    {
      CC1101_SpiWriteReg(CC1101_TEST0, 0x0B);
    }
    else
    {
      CC1101_SpiWriteReg(CC1101_TEST0, 0x09);
      int s = CC1101_SpiReadStatus(CC1101_FSCAL2);
      if (s < 32)
      {
        CC1101_SpiWriteReg(CC1101_FSCAL2, s + 32);
      }
      if (last_pa != 1)
      {
        CC1101_setPA(pa);
      }
    }
  }
  else if (MHz >= 378 && MHz <= 464)
  {
    CC1101_SpiWriteReg(CC1101_FSCTRL0, map(MHz, 378, 464, clb2[0], clb2[1]));
    if (MHz < 430.5)
    {
      CC1101_SpiWriteReg(CC1101_TEST0, 0x0B);
    }
    else
    {
      CC1101_SpiWriteReg(CC1101_TEST0, 0x09);
      int s = CC1101_SpiReadStatus(CC1101_FSCAL2);
      if (s < 32)
      {
        CC1101_SpiWriteReg(CC1101_FSCAL2, s + 32);
      }
      if (last_pa != 2)
      {
        CC1101_setPA(pa);
      }
    }
  }
  else if (MHz >= 779 && MHz <= 899.99)
  {
    CC1101_SpiWriteReg(CC1101_FSCTRL0, map(MHz, 779, 899, clb3[0], clb3[1]));
    if (MHz < 861)
    {
      CC1101_SpiWriteReg(CC1101_TEST0, 0x0B);
    }
    else
    {
      CC1101_SpiWriteReg(CC1101_TEST0, 0x09);
      int s = CC1101_SpiReadStatus(CC1101_FSCAL2);
      if (s < 32)
      {
        CC1101_SpiWriteReg(CC1101_FSCAL2, s + 32);
      }
      if (last_pa != 3)
      {
        CC1101_setPA(pa);
      }
    }
  }
  else if (MHz >= 900 && MHz <= 928)
  {
    CC1101_SpiWriteReg(CC1101_FSCTRL0, map(MHz, 900, 928, clb4[0], clb4[1]));
    CC1101_SpiWriteReg(CC1101_TEST0, 0x09);
    int s = CC1101_SpiReadStatus(CC1101_FSCAL2);
    if (s < 32)
    {
      CC1101_SpiWriteReg(CC1101_FSCAL2, s + 32);
    }
    if (last_pa != 4)
    {
      CC1101_setPA(pa);
    }
  }
}
/****************************************************************
 *FUNCTION NAME:Calibration offset
 *FUNCTION     :Set calibration offset
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setClb(byte b, byte s, byte e)
{
  if (b == 1)
  {
    clb1[0] = s;
    clb1[1] = e;
  }
  else if (b == 2)
  {
    clb2[0] = s;
    clb2[1] = e;
  }
  else if (b == 3)
  {
    clb3[0] = s;
    clb3[1] = e;
  }
  else if (b == 4)
  {
    clb4[0] = s;
    clb4[1] = e;
  }
}
/****************************************************************
 *FUNCTION NAME:CC1101_getCC1101
 *FUNCTION     :Test Spi connection and return 1 when true.
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
bool CC1101_getCC1101(void)
{
  // setSpi();
  if (CC1101_SpiReadStatus(0x31) > 0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}
/****************************************************************
 *FUNCTION NAME:CC1101_getMode
 *FUNCTION     :Return the Mode. Sidle = 0, TX = 1, Rx = 2.
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
byte CC1101_getMode(void)
{
  return trxstate;
}
/****************************************************************
 *FUNCTION NAME:Set Sync_Word
 *FUNCTION     :Sync Word
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setSyncWord(byte sh, byte sl)
{
  CC1101_SpiWriteReg(CC1101_SYNC1, sh);
  CC1101_SpiWriteReg(CC1101_SYNC0, sl);
}
/****************************************************************
 *FUNCTION NAME:Set ADDR
 *FUNCTION     :Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setAddr(byte v)
{
  CC1101_SpiWriteReg(CC1101_ADDR, v);
}
/****************************************************************
 *FUNCTION NAME:Set PQT
 *FUNCTION     :Preamble quality estimator threshold
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setPQT(byte v)
{
  CC1101_Split_PKTCTRL1();
  pc1PQT = 0;
  if (v > 7)
  {
    v = 7;
  }
  pc1PQT = v * 32;
  CC1101_SpiWriteReg(CC1101_PKTCTRL1, pc1PQT + pc1CRC_AF + pc1APP_ST + pc1ADRCHK);
}
/****************************************************************
 *FUNCTION NAME:Set CRC_AUTOFLUSH
 *FUNCTION     :Enable automatic flush of RX FIFO when CRC is not OK
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setCRC_AF(bool v)
{
  CC1101_Split_PKTCTRL1();
  pc1CRC_AF = 0;
  if (v == 1)
  {
    pc1CRC_AF = 8;
  }
  CC1101_SpiWriteReg(CC1101_PKTCTRL1, pc1PQT + pc1CRC_AF + pc1APP_ST + pc1ADRCHK);
}
/****************************************************************
 *FUNCTION NAME:Set APPEND_STATUS
 *FUNCTION     :When enabled, two status bytes will be appended to the payload of the packet
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setAppendStatus(bool v)
{
  CC1101_Split_PKTCTRL1();
  pc1APP_ST = 0;
  if (v == 1)
  {
    pc1APP_ST = 4;
  }
  CC1101_SpiWriteReg(CC1101_PKTCTRL1, pc1PQT + pc1CRC_AF + pc1APP_ST + pc1ADRCHK);
}
/****************************************************************
 *FUNCTION NAME:Set ADR_CHK
 *FUNCTION     :Controls address check configuration of received packages
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setAdrChk(byte v)
{
  CC1101_Split_PKTCTRL1();
  pc1ADRCHK = 0;
  if (v > 3)
  {
    v = 3;
  }
  pc1ADRCHK = v;
  CC1101_SpiWriteReg(CC1101_PKTCTRL1, pc1PQT + pc1CRC_AF + pc1APP_ST + pc1ADRCHK);
}
/****************************************************************
 *FUNCTION NAME:Set WHITE_DATA
 *FUNCTION     :Turn data whitening on / off.
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setWhiteData(bool v)
{
  CC1101_Split_PKTCTRL0();
  pc0WDATA = 0;
  if (v == 1)
  {
    pc0WDATA = 64;
  }
  CC1101_SpiWriteReg(CC1101_PKTCTRL0, pc0WDATA + pc0PktForm + pc0CRC_EN + pc0LenConf);
}
/****************************************************************
 *FUNCTION NAME:Set PKT_FORMAT
 *FUNCTION     :Format of RX and TX data
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setPktFormat(byte v)
{
  CC1101_Split_PKTCTRL0();
  pc0PktForm = 0;
  if (v > 3)
  {
    v = 3;
  }
  pc0PktForm = v * 16;
  CC1101_SpiWriteReg(CC1101_PKTCTRL0, pc0WDATA + pc0PktForm + pc0CRC_EN + pc0LenConf);
}
/****************************************************************
 *FUNCTION NAME:Set CRC
 *FUNCTION     :CRC calculation in TX and CRC check in RX
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setCrc(bool v)
{
  CC1101_Split_PKTCTRL0();
  pc0CRC_EN = 0;
  if (v == 1)
  {
    pc0CRC_EN = 4;
  }
  CC1101_SpiWriteReg(CC1101_PKTCTRL0, pc0WDATA + pc0PktForm + pc0CRC_EN + pc0LenConf);
}
/****************************************************************
 *FUNCTION NAME:Set LENGTH_CONFIG
 *FUNCTION     :Configure the packet length
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setLengthConfig(byte v)
{
  CC1101_Split_PKTCTRL0();
  pc0LenConf = 0;
  if (v > 3)
  {
    v = 3;
  }
  pc0LenConf = v;
  CC1101_SpiWriteReg(CC1101_PKTCTRL0, pc0WDATA + pc0PktForm + pc0CRC_EN + pc0LenConf);
}
/****************************************************************
 *FUNCTION NAME:Set PACKET_LENGTH
 *FUNCTION     :Indicates the packet length
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setPacketLength(byte v)
{
  CC1101_SpiWriteReg(CC1101_PKTLEN, v);
}
/****************************************************************
 *FUNCTION NAME:Set DCFILT_OFF
 *FUNCTION     :Disable digital DC blocking filter before demodulator
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setDcFilterOff(bool v)
{
  CC1101_Split_MDMCFG2();
  m2DCOFF = 0;
  if (v == 1)
  {
    m2DCOFF = 128;
  }
  CC1101_SpiWriteReg(CC1101_MDMCFG2, m2DCOFF + m2MODFM + m2MANCH + m2SYNCM);
}
/****************************************************************
 *FUNCTION NAME:Set MANCHESTER
 *FUNCTION     :Enables Manchester encoding/decoding
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setManchester(bool v)
{
  CC1101_Split_MDMCFG2();
  m2MANCH = 0;
  if (v == 1)
  {
    m2MANCH = 8;
  }
  CC1101_SpiWriteReg(CC1101_MDMCFG2, m2DCOFF + m2MODFM + m2MANCH + m2SYNCM);
}
/****************************************************************
 *FUNCTION NAME:Set SYNC_MODE
 *FUNCTION     :Combined sync-word qualifier mode
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setSyncMode(byte v)
{
  CC1101_Split_MDMCFG2();
  m2SYNCM = 0;
  if (v > 7)
  {
    v = 7;
  }
  m2SYNCM = v;
  CC1101_SpiWriteReg(CC1101_MDMCFG2, m2DCOFF + m2MODFM + m2MANCH + m2SYNCM);
}
/****************************************************************
 *FUNCTION NAME:Set FEC
 *FUNCTION     :Enable Forward Error Correction (FEC)
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setFEC(bool v)
{
  CC1101_Split_MDMCFG1();
  m1FEC = 0;
  if (v == 1)
  {
    m1FEC = 128;
  }
  CC1101_SpiWriteReg(CC1101_MDMCFG1, m1FEC + m1PRE + m1CHSP);
}
/****************************************************************
 *FUNCTION NAME:Set PRE
 *FUNCTION     :Sets the minimum number of preamble bytes to be transmitted.
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setPRE(byte v)
{
  CC1101_Split_MDMCFG1();
  m1PRE = 0;
  if (v > 7)
  {
    v = 7;
  }
  m1PRE = v * 16;
  CC1101_SpiWriteReg(CC1101_MDMCFG1, m1FEC + m1PRE + m1CHSP);
}
/****************************************************************
 *FUNCTION NAME:Set Channel
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setChannel(byte ch)
{
  chan = ch;
  CC1101_SpiWriteReg(CC1101_CHANNR, chan);
}
/****************************************************************
 *FUNCTION NAME:Set Channel spacing
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setChsp(float f)
{
  CC1101_Split_MDMCFG1();
  byte MDMCFG0 = 0;
  m1CHSP = 0;
  if (f > 405.456543)
  {
    f = 405.456543;
  }
  if (f < 25.390625)
  {
    f = 25.390625;
  }
  for (int i = 0; i < 5; i++)
  {
    if (f <= 50.682068)
    {
      f -= 25.390625;
      f /= 0.0991825;
      MDMCFG0 = f;
      float s1 = (f - MDMCFG0) * 10;
      if (s1 >= 5)
      {
        MDMCFG0++;
      }
      i = 5;
    }
    else
    {
      m1CHSP++;
      f /= 2;
    }
  }
  CC1101_SpiWriteReg(19, m1CHSP + m1FEC + m1PRE);
  CC1101_SpiWriteReg(20, MDMCFG0);
}
/****************************************************************
 *FUNCTION NAME:Set Receive bandwidth
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setRxBW(float f)
{
  CC1101_Split_MDMCFG4();
  int s1 = 3;
  int s2 = 3;
  for (int i = 0; i < 3; i++)
  {
    if (f > 101.5625)
    {
      f /= 2;
      s1--;
    }
    else
    {
      i = 3;
    }
  }
  for (int i = 0; i < 3; i++)
  {
    if (f > 58.1)
    {
      f /= 1.25;
      s2--;
    }
    else
    {
      i = 3;
    }
  }
  s1 *= 64;
  s2 *= 16;
  m4RxBw = s1 + s2;
  CC1101_SpiWriteReg(16, m4RxBw + m4DaRa);
}
/****************************************************************
 *FUNCTION NAME:Set Data Rate
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setDRate(float d)
{
  CC1101_Split_MDMCFG4();
  float c = d;
  byte MDMCFG3 = 0;
  if (c > 1621.83)
  {
    c = 1621.83;
  }
  if (c < 0.0247955)
  {
    c = 0.0247955;
  }
  m4DaRa = 0;
  for (int i = 0; i < 20; i++)
  {
    if (c <= 0.0494942)
    {
      c = c - 0.0247955;
      c = c / 0.00009685;
      MDMCFG3 = c;
      float s1 = (c - MDMCFG3) * 10;
      if (s1 >= 5)
      {
        MDMCFG3++;
      }
      i = 20;
    }
    else
    {
      m4DaRa++;
      c = c / 2;
    }
  }
  CC1101_SpiWriteReg(16, m4RxBw + m4DaRa);
  CC1101_SpiWriteReg(17, MDMCFG3);
}
/****************************************************************
 *FUNCTION NAME:Set Devitation
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setDeviation(float d)
{
  float f = 1.586914;
  float v = 0.19836425;
  int c = 0;
  if (d > 380.859375)
  {
    d = 380.859375;
  }
  if (d < 1.586914)
  {
    d = 1.586914;
  }
  for (int i = 0; i < 255; i++)
  {
    f += v;
    if (c == 7)
    {
      v *= 2;
      c = -1;
      i += 8;
    }
    if (f >= d)
    {
      c = i;
      i = 255;
    }
    c++;
  }
  CC1101_SpiWriteReg(21, c);
}
/****************************************************************
 *FUNCTION NAME:Split PKTCTRL0
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_Split_PKTCTRL1(void)
{
  int calc = CC1101_SpiReadStatus(7);
  pc1PQT = 0;
  pc1CRC_AF = 0;
  pc1APP_ST = 0;
  pc1ADRCHK = 0;
  for (bool i = 0; i == 0;)
  {
    if (calc >= 32)
    {
      calc -= 32;
      pc1PQT += 32;
    }
    else if (calc >= 8)
    {
      calc -= 8;
      pc1CRC_AF += 8;
    }
    else if (calc >= 4)
    {
      calc -= 4;
      pc1APP_ST += 4;
    }
    else
    {
      pc1ADRCHK = calc;
      i = 1;
    }
  }
}
/****************************************************************
 *FUNCTION NAME:Split PKTCTRL0
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_Split_PKTCTRL0(void)
{
  int calc = CC1101_SpiReadStatus(8);
  pc0WDATA = 0;
  pc0PktForm = 0;
  pc0CRC_EN = 0;
  pc0LenConf = 0;
  for (bool i = 0; i == 0;)
  {
    if (calc >= 64)
    {
      calc -= 64;
      pc0WDATA += 64;
    }
    else if (calc >= 16)
    {
      calc -= 16;
      pc0PktForm += 16;
    }
    else if (calc >= 4)
    {
      calc -= 4;
      pc0CRC_EN += 4;
    }
    else
    {
      pc0LenConf = calc;
      i = 1;
    }
  }
}
/****************************************************************
 *FUNCTION NAME:Split MDMCFG1
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_Split_MDMCFG1(void)
{
  int calc = CC1101_SpiReadStatus(19);
  m1FEC = 0;
  m1PRE = 0;
  m1CHSP = 0;

  for (bool i = 0; i == 0;)
  {
    if (calc >= 128)
    {
      calc -= 128;
      m1FEC += 128;
    }
    else if (calc >= 16)
    {
      calc -= 16;
      m1PRE += 16;
    }
    else
    {
      m1CHSP = calc;
      i = 1;
    }
  }
}
/****************************************************************
 *FUNCTION NAME:Split MDMCFG2
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_Split_MDMCFG2(void)
{
  int calc = CC1101_SpiReadStatus(18);
  m2DCOFF = 0;
  m2MODFM = 0;
  m2MANCH = 0;
  m2SYNCM = 0;
  for (bool i = 0; i == 0;)
  {
    if (calc >= 128)
    {
      calc -= 128;
      m2DCOFF += 128;
    }
    else if (calc >= 16)
    {
      calc -= 16;
      m2MODFM += 16;
    }
    else if (calc >= 8)
    {
      calc -= 8;
      m2MANCH += 8;
    }
    else
    {
      m2SYNCM = calc;
      i = 1;
    }
  }
}
/****************************************************************
 *FUNCTION NAME:Split MDMCFG4
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_Split_MDMCFG4(void)
{
  int calc = CC1101_SpiReadStatus(16);
  m4RxBw = 0;
  m4DaRa = 0;
  for (bool i = 0; i == 0;)
  {
    if (calc >= 64)
    {
      calc -= 64;
      m4RxBw += 64;
    }
    else if (calc >= 16)
    {
      calc -= 16;
      m4RxBw += 16;
    }
    else
    {
      m4DaRa = calc;
      i = 1;
    }
  }
}
/****************************************************************
 *FUNCTION NAME:CC1101_RegConfigSettings
 *FUNCTION     :CC1101 register config //details refer datasheet of CC1101/CC1100//
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_RegConfigSettings(void)
{
  CC1101_SpiWriteReg(CC1101_FSCTRL1, 0x06);

  CC1101_setCCMode(ccmode);
  CC1101_setMHZ(MHz);

  CC1101_SpiWriteReg(CC1101_MDMCFG1, 0x02);
  CC1101_SpiWriteReg(CC1101_MDMCFG0, 0xF8);
  CC1101_SpiWriteReg(CC1101_CHANNR, chan);
  CC1101_SpiWriteReg(CC1101_DEVIATN, 0x47);
  CC1101_SpiWriteReg(CC1101_FREND1, 0x56);
  CC1101_SpiWriteReg(CC1101_MCSM0, 0x18);
  CC1101_SpiWriteReg(CC1101_FOCCFG, 0x16);
  CC1101_SpiWriteReg(CC1101_BSCFG, 0x1C);
  CC1101_SpiWriteReg(CC1101_AGCCTRL2, 0xC7);
  CC1101_SpiWriteReg(CC1101_AGCCTRL1, 0x00);
  CC1101_SpiWriteReg(CC1101_AGCCTRL0, 0xB2);
  CC1101_SpiWriteReg(CC1101_FSCAL3, 0xE9);
  CC1101_SpiWriteReg(CC1101_FSCAL2, 0x2A);
  CC1101_SpiWriteReg(CC1101_FSCAL1, 0x00);
  CC1101_SpiWriteReg(CC1101_FSCAL0, 0x1F);
  CC1101_SpiWriteReg(CC1101_FSTEST, 0x59);
  CC1101_SpiWriteReg(CC1101_TEST2, 0x81);
  CC1101_SpiWriteReg(CC1101_TEST1, 0x35);
  CC1101_SpiWriteReg(CC1101_TEST0, 0x09);
  CC1101_SpiWriteReg(CC1101_PKTCTRL1, 0x04);
  CC1101_SpiWriteReg(CC1101_ADDR, 0x00);
  CC1101_SpiWriteReg(CC1101_PKTLEN, 0x00);
}
/****************************************************************
 *FUNCTION NAME:CC1101_SetTx
 *FUNCTION     :set CC1101 send data
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_SetTx(void)
{
  CC1101_SpiStrobe(CC1101_SIDLE);
  CC1101_SpiStrobe(CC1101_STX); // start send
  trxstate = 1;
}
/****************************************************************
 *FUNCTION NAME:CC1101_SetRx
 *FUNCTION     :set CC1101 to receive state
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_SetRx(void)
{
  CC1101_SpiStrobe(CC1101_SIDLE);
  CC1101_SpiStrobe(CC1101_SRX); // start receive
  trxstate = 2;
}
/****************************************************************
 *FUNCTION NAME:CC1101_SetTx
 *FUNCTION     :set CC1101 send data and change frequency
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
// void CC1101_SetTx(float mhz)
// {
//   CC1101_SpiStrobe(CC1101_SIDLE);
//   CC1101_setMHZ(mhz);
//   CC1101_SpiStrobe(CC1101_STX);        //start send
//   trxstate=1;
// }
/****************************************************************
 *FUNCTION NAME:CC1101_SetRx
 *FUNCTION     :set CC1101 to receive state and change frequency
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
// void CC1101_SetRx(float mhz)
// {
//   CC1101_SpiStrobe(CC1101_SIDLE);
//   CC1101_setMHZ(mhz);
//   CC1101_SpiStrobe(CC1101_SRX);        //start receive
//   trxstate=2;
// }
/****************************************************************
 *FUNCTION NAME:RSSI Level
 *FUNCTION     :Calculating the RSSI Level
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
int CC1101_getRssi(void)
{
  int rssi;
  rssi = CC1101_SpiReadStatus(CC1101_RSSI);
  if (rssi >= 128)
  {
    rssi = (rssi - 256) / 2 - 74;
  }
  else
  {
    rssi = (rssi / 2) - 74;
  }
  return rssi;
}
/****************************************************************
 *FUNCTION NAME:LQI Level
 *FUNCTION     :get Lqi state
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
byte CC1101_getLqi(void)
{
  byte lqi;
  lqi = CC1101_SpiReadStatus(CC1101_LQI);
  return lqi;
}
/****************************************************************
 *FUNCTION NAME:CC1101_setSres
 *FUNCTION     :Reset CC1101
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setSres(void)
{
  CC1101_SpiStrobe(CC1101_SRES);
  trxstate = 0;
}
/****************************************************************
 *FUNCTION NAME:CC1101_setSidle
 *FUNCTION     :set Rx / TX Off
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_setSidle(void)
{
  CC1101_SpiStrobe(CC1101_SIDLE);
  trxstate = 0;
}
/****************************************************************
 *FUNCTION NAME:CC1101_goSleep
 *FUNCTION     :set cc1101 Sleep on
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
void CC1101_goSleep(void)
{
  trxstate = 0;
  CC1101_SpiStrobe(0x36); // Exit RX / TX, turn off frequency synthesizer and exit
  CC1101_SpiStrobe(0x39); // Enter power down mode when CSn goes high.
}
/****************************************************************
 *FUNCTION NAME:Char direct CC1101_SendData
 *FUNCTION     :use CC1101 send data
 *INPUT        :txBuffer: data array to send; size: number of data to send, no more than 61
 *OUTPUT       :none
 ****************************************************************/
void CC1101_SendData1(char *txchar)
{
  int len = strlen(txchar);
  byte chartobyte[len];
  for (int i = 0; i < len; i++)
  {
    chartobyte[i] = txchar[i];
  }
  CC1101_SendData(chartobyte, len);
}
/****************************************************************
 *FUNCTION NAME:CC1101_SendData
 *FUNCTION     :use CC1101 send data
 *INPUT        :txBuffer: data array to send; size: number of data to send, no more than 61
 *OUTPUT       :none
 ****************************************************************/
void CC1101_SendData(byte *txBuffer, byte size)
{
  CC1101_SpiWriteReg(CC1101_TXFIFO, size);
  CC1101_SpiWriteBurstReg(CC1101_TXFIFO, txBuffer, size); // write data to send
  CC1101_SpiStrobe(CC1101_SIDLE);
  CC1101_SpiStrobe(CC1101_STX); // start send
  while (!digitalRead(GDO0))
    ; // Wait for GDO0 to be set -> sync transmitted
  while (digitalRead(GDO0))
    ;                     // Wait for GDO0 to be cleared -> end of packet
  CC1101_SpiStrobe(CC1101_SFTX); // flush TXfifo
  trxstate = 1;
}
/****************************************************************
 *FUNCTION NAME:Char direct CC1101_SendData
 *FUNCTION     :use CC1101 send data without GDO
 *INPUT        :txBuffer: data array to send; size: number of data to send, no more than 61
 *OUTPUT       :none
 ****************************************************************/
void CC1101_SendData3(char *txchar, int t)
{
  int len = strlen(txchar);
  byte chartobyte[len];
  for (int i = 0; i < len; i++)
  {
    chartobyte[i] = txchar[i];
  }
  CC1101_SendData2(chartobyte, len, t);
}
/****************************************************************
 *FUNCTION NAME:CC1101_SendData
 *FUNCTION     :use CC1101 send data without GDO
 *INPUT        :txBuffer: data array to send; size: number of data to send, no more than 61
 *OUTPUT       :none
 ****************************************************************/
void CC1101_SendData2(byte *txBuffer, byte size, int t)
{
  CC1101_SpiWriteReg(CC1101_TXFIFO, size);
  CC1101_SpiWriteBurstReg(CC1101_TXFIFO, txBuffer, size); // write data to send
  CC1101_SpiStrobe(CC1101_SIDLE);
  CC1101_SpiStrobe(CC1101_STX); // start send
  vTaskDelay(t / portTICK_PERIOD_MS);
  CC1101_SpiStrobe(CC1101_SFTX); // flush TXfifo
  trxstate = 1;
}
/****************************************************************
 *FUNCTION NAME:Check CRC
 *FUNCTION     :none
 *INPUT        :none
 *OUTPUT       :none
 ****************************************************************/
bool CC1101_CheckCRC(void)
{
  byte lqi = CC1101_SpiReadStatus(CC1101_LQI);
  bool crc_ok = bitRead(lqi, 7);
  if (crc_ok == 1)
  {
    return 1;
  }
  else
  {
    CC1101_SpiStrobe(CC1101_SFRX);
    CC1101_SpiStrobe(CC1101_SRX);
    return 0;
  }
}
/****************************************************************
 *FUNCTION NAME:CC1101_CheckRxFifo
 *FUNCTION     :check receive data or not
 *INPUT        :none
 *OUTPUT       :flag: 0 no data; 1 receive data
 ****************************************************************/
bool CC1101_CheckRxFifo(int t)
{
  if (trxstate != 2)
  {
    CC1101_SetRx();
  }
  if (CC1101_SpiReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO)
  {
    vTaskDelay(t / portTICK_PERIOD_MS);
    return 1;
  }
  else
  {
    return 0;
  }
}
/****************************************************************
 *FUNCTION NAME:CC1101_CheckReceiveFlag
 *FUNCTION     :check receive data or not
 *INPUT        :none
 *OUTPUT       :flag: 0 no data; 1 receive data
 ****************************************************************/
byte CC1101_CheckReceiveFlag(void)
{
  if (trxstate != 2)
  {
    CC1101_SetRx();
  }
  if (digitalRead(GDO0)) // receive data
  {
    while (digitalRead(GDO0))
      ;
    return 1;
  }
  else // no data
  {
    return 0;
  }
}
/****************************************************************
 *FUNCTION NAME:CC1101_ReceiveData
 *FUNCTION     :read data received from RXfifo
 *INPUT        :rxBuffer: buffer to store data
 *OUTPUT       :size of data received
 ****************************************************************/
byte CC1101_ReceiveData(byte *rxBuffer)
{
  byte size;
  byte status[2];
  byte ret = CC1101_SpiReadStatus(CC1101_RXBYTES);
  printf("%d\n", ret);
  if (ret & BYTES_IN_RXFIFO) // 0x00 & 0x7F
  {

    size = CC1101_SpiReadReg(CC1101_RXFIFO);
    CC1101_SpiReadBurstReg(CC1101_RXFIFO, rxBuffer, size);
    CC1101_SpiReadBurstReg(CC1101_RXFIFO, status, 2);
    CC1101_SpiStrobe(CC1101_SFRX);
    CC1101_SpiStrobe(CC1101_SRX);
    return size;
  }
  else
  {
    CC1101_SpiStrobe(CC1101_SFRX);
    CC1101_SpiStrobe(CC1101_SRX);
    return 0;
  }
}

byte CC1101_getVersion(void)
{
  return CC1101_SpiReadStatus(0x31);
}

byte CC1101_getPartnum(void)
{
  return CC1101_SpiReadStatus(0x30);
}
// esp_err_t ELECHOUSE_CC1101::getData(receivedData_t* data){
//   if (NULL == data)
//     {
//         return ESP_FAIL;
//     }
//     data->rbl=mySwitch.getReceivedBitlength();
//     data->rv=mySwitch.getReceivedValue();
//    if (mySwitch.available()) {
//      printf("Received value:%lu\n",mySwitch.getReceivedValue());

//      printf("Received value Bit Length:%u\n",mySwitch.getReceivedBitlength());
//     mySwitch.resetAvailable();
//   //}
//   vTaskDelay(10/portTICK_PERIOD_MS);

// }
// return ESP_OK;
// }

// esp_err_t setup(){
//   esp_err_t ret=Init(); // initialize the cc1101
//   if(ret!=ESP_OK)
//   return ESP_FAIL;

//   int pin=9;
//   setSpiPin(13, 11, 12, 14); // Set custom pins: (SCK, MISO, MOSI, CSN). Must be BEFORE Init()!
//   setGDO(10, 9); // Set custom GDO pins: (GDO0, GDO2). Gdo0 (tx) and Gdo2 (rx). Must be BEFORE Init();

//   //ELECHOUSE_cc1101.CC1101_setRxBW(812.50);  // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.
//   //ELECHOUSE_cc1101.CC1101_setPA(10); // set TxPower. The following settings are possible depending on the frequency band. Possible values: (-30, -20, -15, -10, -6, 0, 5, 7, 10, 11, 12). Default is max!
//   CC1101_setMHZ(433.92); // Set basic frequency. The lib calculates the frequency automatically (default: 433.92). CC1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.

//   enableReceive(pin,);  // Receiver on interrupt 0

//   CC1101_SetRx();  // set Receive on
//   ESP_LOGI(TAG,"RX MODE");
//   // Check the CC1101 Spi connection.
//   if (CC1101_getCC1101()) {
//    ESP_LOGI(TAG,"CC1101: Connection OK.");
//   }
//   else {
//    ESP_LOGI(TAG,"CC1101: Connection error.");
//   }
//   return ESP_OK;
// }

void CC1101_Transmitting(unsigned long t_code)
{
  enableTransmit(CC1101_gpios->gdo0, &var);
  // cc1101 set Transmit on
  // CC1101_SetTx();
  sendC(t_code, 24, &var);
  // CC1101_SetRx();  // set Receive on
}

// ELECHOUSE_CC1101 ELECHOUSE_cc1101;
// esp_err_t spi_init() {
//  esp_err_t ret = ESP_OK;
// gpio_reset_pin((gpio_num_t)SS_PIN);
// gpio_set_direction((gpio_num_t)SS_PIN, GPIO_MODE_OUTPUT);
// gpio_set_level((gpio_num_t)SS_PIN, 1);
// 	spi_bus_config_t buscfg = {

// 		.mosi_io_num = MOSI_PIN, // set SPI MOSI pin
// 		.miso_io_num = MISO_PIN, // set SPI MISO pin
//     .sclk_io_num = SCK_PIN, // set SPI CLK pin
// 		.quadwp_io_num = -1,
// 		.quadhd_io_num = -1
// 	};

// 	ret = spi_bus_initialize( HOST_ID, &buscfg, SPI_DMA_CH_AUTO );
//   ESP_RETURN_ON_ERROR(ret, TAG, "failed to initialize SPI bus");
// 	ESP_LOGI(TAG, "spi_bus_initialize=%d",ret);
// 	assert(ret==ESP_OK);

// 	spi_device_interface_config_t devcfg =
// 	{
//     .mode = 0, // SPI mode 0
// 		.clock_speed_hz = 5000000, // SPI clock is 5 MHz!
//     .spics_io_num = -1, // we will use manual CS control
//     .flags = SPI_DEVICE_NO_DUMMY,
// 		.queue_size =7
// 	};

// 	ret = spi_bus_add_device( HOST_ID, &devcfg, &_handle);
//   ESP_RETURN_ON_ERROR(ret, TAG, "failed to add device on bus");
// 	ESP_LOGI(TAG, "spi_bus_add_device=%d",ret);
// 	// assert(ret==ESP_OK);

// 	return ret;
// }

uint8_t CC1101_spi_transfer(uint8_t address)
{
  uint8_t datain[1];
  uint8_t dataout[1];
  dataout[0] = address;
  // spi_write_byte(dev, dataout, 1 );
  // spi_read_byte(datain, dataout, 1 );

  spi_transaction_t SPITransaction;
  memset(&SPITransaction, 0, sizeof(spi_transaction_t));
  SPITransaction.length = 8;
  SPITransaction.tx_buffer = dataout;
  SPITransaction.rx_buffer = datain;
  spi_device_transmit(transReceive_t->handle, &SPITransaction);

  return datain[0];
}

float CC1101_sweepFrequency()
{
  ESP_LOGI(TAG, "Frequency scanner Ready");
  float loopFrequency;
  for (int i = 0; i <= 3; i++)
  {
    loopFrequency = CC1101_searchFrequencyForRange(300, 347, 0.1, true);
    ESP_LOGI(TAG, "loopFrequency:315 =%.2f ", loopFrequency);
    if (loopFrequency != -1)
    {
      return loopFrequency;
    }
    loopFrequency = CC1101_searchFrequencyForRange(388, 463, 0.1, true);
    ESP_LOGI(TAG, "loopFrequency:433 =%.2f ", loopFrequency);
    if (loopFrequency != -1)
    {
      return loopFrequency;
    }
    loopFrequency = CC1101_searchFrequencyForRange(779, 927, 0.1, true);
    ESP_LOGI(TAG, "loopFrequency:868 =%.2f ", loopFrequency);
    if (loopFrequency != -1)
    {
      return loopFrequency;
    }
  }
  return 0;
}

float CC1101_searchFrequency(float mhz, float freqSpace, bool isDetails)
{
  float start_freq = mhz;
  float stop_freq = mhz + 1;
  float freq = mhz;
  long compare_freq = 0;
  float mark_freq = 0;
  int rssi = 0;
  int mark_rssi = -60;
  int searchTime = 0;
  // CC1101_setMHZ(mhz);
  // CC1101_setRxBW(58.03);
  CC1101_SetRx();
  vTaskDelay(20 / portTICK_PERIOD_MS);
  // ESP_LOGI(TAG,"Frequency scanner Ready");
  while (1)
  {
    searchTime++;

    CC1101_setMHZ(freq);
    rssi = CC1101_getRssi();
    if (rssi > -60)
    {
      if (rssi > mark_rssi)
      {
        mark_rssi = rssi;
        mark_freq = freq;
      }
    }
    freq += freqSpace;

    if (freq > stop_freq)
    {
      freq = start_freq;
      if (mark_rssi > -60)
      {
        long fr = mark_freq * 100;
        if (fr == compare_freq)
        {
          ESP_LOGI(TAG, "Freq: %f ", mark_freq);
          ESP_LOGI(TAG, "Rssi: %d ", mark_rssi);
          float saveMarkFreq = mark_freq;
          mark_rssi = -60;
          compare_freq = 0;
          mark_freq = 0;
          if (isDetails)
          {
            return saveMarkFreq;
          }
          else
          {
            float startSaveMarkFreq = (int)saveMarkFreq - 3;
            float endSaveMarkFreq = (int)saveMarkFreq + 3;
            float getDetailsRequency = CC1101_searchFrequencyForRange(startSaveMarkFreq, endSaveMarkFreq, 0.01, true);
            return getDetailsRequency;
          }
        }
        else
        {
          compare_freq = mark_freq * 100;
          freq = mark_freq - 0.10;
          mark_freq = 0;
          mark_rssi = -60;
        }
      }
      else
      {
        return -1;
      }
    }
  }
}

float CC1101_searchFrequencyForRange(float startMhz, float endMhz, float freqSpace, bool isDetails)
{
  while (1)
  {
    float frequency = CC1101_searchFrequency(startMhz, freqSpace, isDetails);

    if (frequency != -1)
    {
      return frequency;
    }
    startMhz++;
    if (startMhz > endMhz)
    {
      return -1;
    }
  }
}

void CC1101_API_function(void *data)
{
  // CC1101_setSidle();
  ESP_LOGI(TAG, "API FUNCTION");
  ESP_LOGI(TAG, "trxstate = %d", trxstate);
  cc1101_data *pData = (cc1101_data *)data;

  if (pData->event == SET_BUTTON)
  {
    CC1101_setMHZ(pData->freq);
    if (trxstate != 2)
    {
      CC1101_SetRx(); // start receive
      printf("RX called");
    }

    ESP_LOGI(TAG, "Programming mode");
    wFlag = true;
    CC1101_setMHZ(pData->freq);
    ESP_LOGI(TAG, "RX MODE");
    // CC1101_SetRx();
    // Check the CC1101 Spi connection.
    if (CC1101_getCC1101())
      ESP_LOGI(TAG, "CC1101: Connection OK.");
    else
      ESP_LOGI(TAG, "CC1101: Connection error.");
    while (wFlag)
    {

      // resetAvailable(&var);
      // vTaskDelay(75/portTICK_PERIOD_MS);
      if (available(&var))
      {
        pData->code = getReceivedValue(&var);
        ESP_LOGI(TAG, "Value stored in button is %lu", pData->code);
        wFlag = false;
        resetAvailable(&var);
        // CC1101_SetTx();
      }
    }
  }
  else if (pData->event == FREQ_SWEEP)
  {
    // CC1101_SetRx();
    // if(trxstate != 2){
    // CC1101_SetRx();       //start receive
    // printf("RX called");
    // }
    pData->freq = CC1101_sweepFrequency();
  }
  else if (pData->event == USER_INPUT)
  {
    CC1101_setMHZ(pData->freq);
    ESP_LOGI(TAG, "Frequency set to %f MHz", pData->freq);
  }
  else if (pData->event == CC1101_Transmitting)
  {
    CC1101_setMHZ(pData->freq);
    CC1101_SetTx(); // start Transmit
    CC1101_setMHZ(pData->freq);
    CC1101_Transmitting(pData->code);
    ESP_LOGI(TAG, "Transmitted");
    // CC1101_SetRx();  // set Receive on
    // trxstate=1;
    // CC1101_goSleep();
  }
  CC1101_setSidle();
}

void CC1101_ezlopi_cc1101_upcall(uint8_t *buffer, s_ezlopi_uart_object_handle_t uart_object_handle, void *user_args)
{
  cc1101_read_upcall(buffer, &data);
  CC1101_API_function(&data);
  CC1101_structToJSON(&data);
}

void cc1101_read_upcall(uint8_t *buffer, cc1101_data *data)
{
  // const char* datas={"event":3,"freq":433.92,"code":12330466};

  char *json_data = (char *)(buffer);
  printf("%s\n", json_data);

  // Parse JSON
  cJSON *root = cJSON_Parse(json_data);
  if (root == NULL)
  {
    printf("Error parsing JSON.\n");
  }
  else
  {
    //   // Get values from JSON
    //   // data->event = cJSON_GetObjectItem(root, "event")->valueint;
    data->event = cJSON_GetObjectItem(root, "event")->valueint;
    data->freq = cJSON_GetObjectItem(root, "freq")->valuedouble;
    data->code = cJSON_GetObjectItem(root, "code")->valueint;

    //   // Print the values
    //   // printf("%s\n",buffer);
    printf("Event: %d\n", data->event);
    printf("Freq: %.2f\n", data->freq);
    printf("Code: %lu\n", data->code);

    //   // Clean up cJSON
    cJSON_Delete(root);
    //   }
  }
}

void CC1101_structToJSON(cc1101_data *data)
{
  cJSON *jsonObject = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonObject, "event", data->event);
  cJSON_AddNumberToObject(jsonObject, "freq", data->freq);
  cJSON_AddNumberToObject(jsonObject, "code", data->code);
  char *jsonString = cJSON_Print(jsonObject);
  uart_write_bytes(UART_NUM_0, jsonString, strlen(jsonString));
  cJSON_Delete(jsonObject);
  free(jsonString);
  jsonString = NULL;
}