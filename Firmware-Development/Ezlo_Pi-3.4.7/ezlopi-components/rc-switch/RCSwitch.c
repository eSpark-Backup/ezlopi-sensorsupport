/*
  RCSwitch - Arduino libary for remote control outlet switches
  Copyright (c) 2011 Suat Özgür.  All right reserved.
  
  Contributors:
  - Andre Koehler / info(at)tomate-online(dot)de
  - Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
  - Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=46
  - Dominik Fischer / dom_fischer(at)web(dot)de
  - Frank Oltmanns / <first name>.<last name>(at)gmail(dot)com
  - Andreas Steinel / A.<lastname>(at)gmail(dot)com
  - Max Horn / max(at)quendi(dot)de
  - Robert ter Vehn / <first name>.<last name>(at)gmail(dot)com
  - Johann Richard / <first name>.<last name>(at)gmail(dot)com
  - Vlad Gheorghe / <first name>.<last name>(at)gmail(dot)com https://github.com/vgheo
  - Matias Cuenca-Acuna 
  
  Project home: https://github.com/sui77/rc-switch/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "rom/ets_sys.h"
#include "RCSwitch.h"
#include "driver/gpio.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "functional.h"
typedef uint8_t byte;

static void gpio_interrupt_handler(void* args);
#define HIGH 1
#define LOW 0
#define OUTPUT GPIO_MODE_OUTPUT
#define INPUT GPIO_MODE_INPUT
#ifdef RaspberryPi
    // PROGMEM and _P functions are for AVR based microprocessors,
    // so we must normalize these for the ARM processor:
    #define PROGMEM
    #define memcpy_P(dest, src, num) memcpy((dest), (src), (num))
#endif

#if defined(ESP8266)
    // interrupt handler and related code must be in RAM on ESP8266,
    // according to issue #46.
    #define RECEIVE_ATTR ICACHE_RAM_ATTR
    #define VAR_ISR_ATTR
#elif defined(ESP32)
    #define RECEIVE_ATTR IRAM_ATTR
    #define VAR_ISR_ATTR DRAM_ATTR
#else
    #define RECEIVE_ATTR IRAM_ATTR
    #define VAR_ISR_ATTR
#endif

// static inline esp_err_t digitalWrite(unsigned char pin, bool state){
// 	return gpio_set_level((gpio_num_t)pin, state);
// }

// static inline esp_err_t pinMode(unsigned char pin, gpio_mode_t mode)
// {
//   return gpio_set_direction((gpio_num_t)pin, mode);
// }

// static inline int digitalRead(unsigned char  pin){
// 	return gpio_get_level((gpio_num_t)pin);
// }

// static inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
//   return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
// }

// static inline bool bitRead(uint8_t x,uint8_t y ){
//     return ((x >> y) & 0x01);
// }



/* Format for protocol definitions:
 * {pulselength, Sync bit, "0" bit, "1" bit, invertedSignal}
 * 
 * pulselength: pulse length in microseconds, e.g. 350
 * Sync bit: {1, 31} means 1 high pulse and 31 low pulses
 *     (perceived as a 31*pulselength long pulse, total length of sync bit is
 *     32*pulselength microseconds), i.e:
 *      _
 *     | |_______________________________ (don't count the vertical bars)
 * "0" bit: waveform for a data bit of value "0", {1, 3} means 1 high pulse
 *     and 3 low pulses, total length (1+3)*pulselength, i.e:
 *      _
 *     | |___
 * "1" bit: waveform for a data bit of value "1", e.g. {3,1}:
 *      ___
 *     |   |_
 *
 * These are combined to form Tri-State bits when sending or receiving codes.
 */
// #if defined(ESP8266) || defined(ESP32)
static const VAR_ISR_ATTR Protocol proto[] = {
// #else
// static const RCSwitch::Protocol PROGMEM proto[] = {
// #endif
  { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false },    // protocol 1
  { 650, {  1, 10 }, {  1,  2 }, {  2,  1 }, false },    // protocol 2
  { 100, { 30, 71 }, {  4, 11 }, {  9,  6 }, false },    // protocol 3
  { 380, {  1,  6 }, {  1,  3 }, {  3,  1 }, false },    // protocol 4
  { 500, {  6, 14 }, {  1,  2 }, {  2,  1 }, false },    // protocol 5
  { 450, { 23,  1 }, {  1,  2 }, {  2,  1 }, true },     // protocol 6 (HT6P20B)
  { 150, {  2, 62 }, {  1,  6 }, {  6,  1 }, false },    // protocol 7 (HS2303-PT, i. e. used in AUKEY Remote)
  { 200, {  3, 130}, {  7, 16 }, {  3,  16}, false},     // protocol 8 Conrad RS-200 RX
  { 200, { 130, 7 }, {  16, 7 }, { 16,  3 }, true},      // protocol 9 Conrad RS-200 TX
  { 365, { 18,  1 }, {  3,  1 }, {  1,  3 }, true },     // protocol 10 (1ByOne Doorbell)
  { 270, { 36,  1 }, {  1,  2 }, {  2,  1 }, true },     // protocol 11 (HT12E)
  { 320, { 36,  1 }, {  1,  2 }, {  2,  1 }, true }      // protocol 12 (SM5212)
};

enum {
   numProto = sizeof(proto) / sizeof(proto[0])
};


// var.nReceivedValue = 0;
// var.nReceivedBitlength = 0;
// var.nReceivedDelay = 0;
// var.nReceivedProtocol = 0;
// var.nReceiveTolerance = 60;
// var.nSeparationLimit = 4300;
// separationLimit: minimum microseconds between received codes, closer codes are ignored.
// according to discussion on issue #14 it might be more suitable to set the separation
// limit to the same time as the 'low' part of the sync signal for the current protocol.
// var.timings[RCSWITCH_MAX_CHANGES];


void RCSwitchInit(Variables* obj) {
  obj->nTransmitterPin = -1;
  setRepeatTransmit(10,obj);
  setProtocolInt(1,obj);

  obj->nReceiverInterrupt = -1;
  setReceiveTolerance(60,obj);
  obj->nReceivedValue = 0;
 
}

/**
  * Sets the protocol to send.
  */
void setProtocol(Protocol protocol,Variables* this) {
  this->protocol = protocol;
}

/**
  * Sets the protocol to send, from a list of predefined protocols
  */
void setProtocolInt(int nProtocol,Variables* this) {
  if (nProtocol < 1 || nProtocol > numProto) {
    nProtocol = 1;  // TODO: trigger an error, e.g. "bad protocol" ???
  }
#if defined(ESP8266) || defined(ESP32)
  this->protocol = proto[nProtocol-1];
#else
  memcpy(&this->protocol, &proto[nProtocol-1], sizeof(Protocol));
#endif
}

/**
  * Sets the protocol to send with pulse length in microseconds.
  */
void setProtocolPulse(int nProtocol, int nPulseLength,Variables* this) {
  setProtocolInt(nProtocol,this);
  setPulseLength(nPulseLength,this);
}


/**
  * Sets pulse length in microseconds
  */
void setPulseLength(int nPulseLength,Variables* this) {
  this->protocol.pulseLength = nPulseLength;
}

/**
 * Sets Repeat Transmits
 */
void setRepeatTransmit(int nRepeatTransmit,Variables* this) {
  this->nRepeatTransmit = nRepeatTransmit;
}

/**
 * Set Receiving Tolerance
 */

void setReceiveTolerance(int nPercent,Variables* obj) {
  obj->nReceiveTolerance = nPercent;
}

  

/**
 * Enable transmissions
 *
 * @param nTransmitterPin    Arduino Pin to which the sender is connected to
 */
void enableTransmit(int nTransmitterPin,Variables* this) {
  this->nTransmitterPin = nTransmitterPin;
  pinMode(this->nTransmitterPin, OUTPUT);
}

/**
  * Disable transmissions
  */
void disableTransmit(Variables* this) {
  this->nTransmitterPin = -1;
}

/**
 * Switch a remote switch on (Type D REV)
 *
 * @param sGroup        Code of the switch group (A,B,C,D)
 * @param nDevice       Number of the switch itself (1..3)
 */
void switchOnD(char sGroup, int nDevice,Variables* this) {
  sendTriState( getCodeWordD(sGroup, nDevice, true) , this);
}

// /**
//  * Switch a remote switch off (Type D REV)
//  *
//  * @param sGroup        Code of the switch group (A,B,C,D)
//  * @param nDevice       Number of the switch itself (1..3)
//  */
void switchOffD(char sGroup, int nDevice,Variables* this) {
  sendTriState( getCodeWordD(sGroup, nDevice, false), this );
}

// /**
//  * Switch a remote switch on (Type C Intertechno)
//  *
//  * @param sFamily  Familycode (a..f)
//  * @param nGroup   Number of group (1..4)
//  * @param nDevice  Number of device (1..4)
//   */
void switchOnF(char sFamily, int nGroup, int nDevice,Variables* this) {
  sendTriState( getCodeWordC(sFamily, nGroup, nDevice, true) , this);
}

// /**
//  * Switch a remote switch off (Type C Intertechno)
//  *
//  * @param sFamily  Familycode (a..f)
//  * @param nGroup   Number of group (1..4)
//  * @param nDevice  Number of device (1..4)
//  */
void switchOffF(char sFamily, int nGroup, int nDevice,Variables* this) {
  sendTriState( getCodeWordC(sFamily, nGroup, nDevice, false), this );
}

// /**
//  * Switch a remote switch on (Type B with two rotary/sliding switches)
//  *
//  * @param nAddressCode  Number of the switch group (1..4)
//  * @param nChannelCode  Number of the switch itself (1..4)
//  */
void switchOnAdd(int nAddressCode, int nChannelCode,Variables* this) {
  sendTriState( getCodeWordB(nAddressCode, nChannelCode, true) , this);
}

// /**
//  * Switch a remote switch off (Type B with two rotary/sliding switches)
//  *
//  * @param nAddressCode  Number of the switch group (1..4)
//  * @param nChannelCode  Number of the switch itself (1..4)
//  */
void switchOffAdd(int nAddressCode, int nChannelCode,Variables* this) {
  sendTriState( getCodeWordB(nAddressCode, nChannelCode, false), this );
}

// /**
//  * Deprecated, use switchOn(const char* sGroup, const char* sDevice) instead!
//  * Switch a remote switch on (Type A with 10 pole DIP switches)
//  *
//  * @param sGroup        Code of the switch group (refers to DIP switches 1..5 where "1" = on and "0" = off, if all DIP switches are on it's "11111")
//  * @param nChannelCode  Number of the switch itself (1..5)
//  */
void switchOn(const char* sGroup, int nChannel,Variables* this) {
  const char* code[6] = { "00000", "10000", "01000", "00100", "00010", "00001" };
  switchOn(sGroup, code[nChannel], this);
}

// /**
//  * Deprecated, use switchOff(const char* sGroup, const char* sDevice) instead!
//  * Switch a remote switch off (Type A with 10 pole DIP switches)
//  *
//  * @param sGroup        Code of the switch group (refers to DIP switches 1..5 where "1" = on and "0" = off, if all DIP switches are on it's "11111")
//  * @param nChannelCode  Number of the switch itself (1..5)
//  */
void switchOff(const char* sGroup, int nChannel,Variables* this) {
  const char* code[6] = { "00000", "10000", "01000", "00100", "00010", "00001" };
  switchOff(sGroup, code[nChannel], this);
}

// /**
//  * Switch a remote switch on (Type A with 10 pole DIP switches)
//  *
//  * @param sGroup        Code of the switch group (refers to DIP switches 1..5 where "1" = on and "0" = off, if all DIP switches are on it's "11111")
//  * @param sDevice       Code of the switch device (refers to DIP switches 6..10 (A..E) where "1" = on and "0" = off, if all DIP switches are on it's "11111")
//  */
void switchOnA(const char* sGroup, const char* sDevice,Variables* this) {
  sendTriState( getCodeWordA(sGroup, sDevice, true) , this);
}

// /**
//  * Switch a remote switch off (Type A with 10 pole DIP switches)
//  *
//  * @param sGroup        Code of the switch group (refers to DIP switches 1..5 where "1" = on and "0" = off, if all DIP switches are on it's "11111")
//  * @param sDevice       Code of the switch device (refers to DIP switches 6..10 (A..E) where "1" = on and "0" = off, if all DIP switches are on it's "11111")
//  */
void switchOffA(const char* sGroup, const char* sDevice,Variables* this) {
  sendTriState( getCodeWordA(sGroup, sDevice, false), this );
}


/**
 * Returns a char[13], representing the code word to be send.
 *
 */
char* getCodeWordA(const char* sGroup, const char* sDevice, bool bStatus) {
  static char sReturn[13];
  int nReturnPos = 0;

  for (int i = 0; i < 5; i++) {
    sReturn[nReturnPos++] = (sGroup[i] == '0') ? 'F' : '0';
  }

  for (int i = 0; i < 5; i++) {
    sReturn[nReturnPos++] = (sDevice[i] == '0') ? 'F' : '0';
  }

  sReturn[nReturnPos++] = bStatus ? '0' : 'F';
  sReturn[nReturnPos++] = bStatus ? 'F' : '0';

  sReturn[nReturnPos] = '\0';
  return sReturn;
}

/**
 * Encoding for type B switches with two rotary/sliding switches.
 *
 * The code word is a tristate word and with following bit pattern:
 *
 * +-----------------------------+-----------------------------+----------+------------+
 * | 4 bits address              | 4 bits address              | 3 bits   | 1 bit      |
 * | switch group                | switch number               | not used | on / off   |
 * | 1=0FFF 2=F0FF 3=FF0F 4=FFF0 | 1=0FFF 2=F0FF 3=FF0F 4=FFF0 | FFF      | on=F off=0 |
 * +-----------------------------+-----------------------------+----------+------------+
 *
 * @param nAddressCode  Number of the switch group (1..4)
 * @param nChannelCode  Number of the switch itself (1..4)
 * @param bStatus       Whether to switch on (true) or off (false)
 *
 * @return char[13], representing a tristate code word of length 12
 */
char* getCodeWordB(int nAddressCode, int nChannelCode, bool bStatus) {
  static char sReturn[13];
  int nReturnPos = 0;

  if (nAddressCode < 1 || nAddressCode > 4 || nChannelCode < 1 || nChannelCode > 4) {
    return 0;
  }

  for (int i = 1; i <= 4; i++) {
    sReturn[nReturnPos++] = (nAddressCode == i) ? '0' : 'F';
  }

  for (int i = 1; i <= 4; i++) {
    sReturn[nReturnPos++] = (nChannelCode == i) ? '0' : 'F';
  }

  sReturn[nReturnPos++] = 'F';
  sReturn[nReturnPos++] = 'F';
  sReturn[nReturnPos++] = 'F';

  sReturn[nReturnPos++] = bStatus ? 'F' : '0';

  sReturn[nReturnPos] = '\0';
  return sReturn;
}

/**
 * Like getCodeWord (Type C = Intertechno)
 */
char* getCodeWordC(char sFamily, int nGroup, int nDevice, bool bStatus) {
  static char sReturn[13];
  int nReturnPos = 0;

  int nFamily = (int)sFamily - 'a';
  if ( nFamily < 0 || nFamily > 15 || nGroup < 1 || nGroup > 4 || nDevice < 1 || nDevice > 4) {
    return 0;
  }
  
  // encode the family into four bits
  sReturn[nReturnPos++] = (nFamily & 1) ? 'F' : '0';
  sReturn[nReturnPos++] = (nFamily & 2) ? 'F' : '0';
  sReturn[nReturnPos++] = (nFamily & 4) ? 'F' : '0';
  sReturn[nReturnPos++] = (nFamily & 8) ? 'F' : '0';

  // encode the device and group
  sReturn[nReturnPos++] = ((nDevice-1) & 1) ? 'F' : '0';
  sReturn[nReturnPos++] = ((nDevice-1) & 2) ? 'F' : '0';
  sReturn[nReturnPos++] = ((nGroup-1) & 1) ? 'F' : '0';
  sReturn[nReturnPos++] = ((nGroup-1) & 2) ? 'F' : '0';

  // encode the status code
  sReturn[nReturnPos++] = '0';
  sReturn[nReturnPos++] = 'F';
  sReturn[nReturnPos++] = 'F';
  sReturn[nReturnPos++] = bStatus ? 'F' : '0';

  sReturn[nReturnPos] = '\0';
  return sReturn;
}

/**
 * Encoding for the REV Switch Type
 *
 * The code word is a tristate word and with following bit pattern:
 *
 * +-----------------------------+-------------------+----------+--------------+
 * | 4 bits address              | 3 bits address    | 3 bits   | 2 bits       |
 * | switch group                | device number     | not used | on / off     |
 * | A=1FFF B=F1FF C=FF1F D=FFF1 | 1=0FF 2=F0F 3=FF0 | 000      | on=10 off=01 |
 * +-----------------------------+-------------------+----------+--------------+
 *
 * Source: http://www.the-intruder.net/funksteckdosen-von-rev-uber-arduino-ansteuern/
 *
 * @param sGroup        Name of the switch group (A..D, resp. a..d) 
 * @param nDevice       Number of the switch itself (1..3)
 * @param bStatus       Whether to switch on (true) or off (false)
 *
 * @return char[13], representing a tristate code word of length 12
 */
char* getCodeWordD(char sGroup, int nDevice, bool bStatus) {
  static char sReturn[13];
  int nReturnPos = 0;

  // sGroup must be one of the letters in "abcdABCD"
  int nGroup = (sGroup >= 'a') ? (int)sGroup - 'a' : (int)sGroup - 'A';
  if ( nGroup < 0 || nGroup > 3 || nDevice < 1 || nDevice > 3) {
    return 0;
  }

  for (int i = 0; i < 4; i++) {
    sReturn[nReturnPos++] = (nGroup == i) ? '1' : 'F';
  }

  for (int i = 1; i <= 3; i++) {
    sReturn[nReturnPos++] = (nDevice == i) ? '1' : 'F';
  }

  sReturn[nReturnPos++] = '0';
  sReturn[nReturnPos++] = '0';
  sReturn[nReturnPos++] = '0';

  sReturn[nReturnPos++] = bStatus ? '1' : '0';
  sReturn[nReturnPos++] = bStatus ? '0' : '1';

  sReturn[nReturnPos] = '\0';
  return sReturn;
}

/**
 * @param sCodeWord   a tristate code word consisting of the letter 0, 1, F
 */
void sendTriState(const char* sCodeWord,Variables* this) {
  // turn the tristate code word into the corresponding bit pattern, then send it
  unsigned long code = 0;
  unsigned int length = 0;
  for (const char* p = sCodeWord; *p; p++) {
    code <<= 2L;
    switch (*p) {
      case '0':
        // bit pattern 00
        break;
      case 'F':
        // bit pattern 01
        code |= 1L;
        break;
      case '1':
        // bit pattern 11
        code |= 3L;
        break;
    }
    length += 2;
  }
  sendC(code, length,this);
}

/**
 * @param sCodeWord   a binary code word consisting of the letter 0, 1
 */
void sendB(const char* sCodeWord, Variables* this) {
  // turn the tristate code word into the corresponding bit pattern, then send it
  unsigned long code = 0;
  unsigned int length = 0;
  for (const char* p = sCodeWord; *p; p++) {
    code <<= 1L;
    if (*p != '0')
      code |= 1L;
    length++;
  }
  sendC(code, length,this);
}

/**
 * Transmit the first 'length' bits of the integer 'code'. The
 * bits are sent from MSB to LSB, i.e., first the bit at position length-1,
 * then the bit at position length-2, and so on, till finally the bit at position 0.
 */
void sendC(unsigned long code, unsigned int length,Variables* this) {
  if (this->nTransmitterPin == -1)
    return;


  // make sure the receiver is disabled while we transmit
  int nReceiverInterrupt_backup = this->nReceiverInterrupt;
  if (nReceiverInterrupt_backup != -1) {
    disableReceive(this);
  }

  
  for (int nRepeat = 0; nRepeat < this->nRepeatTransmit; nRepeat++) {
    for (int i = length-1; i >= 0; i--) {
      if (code & (1L << i))
        transmit(this->protocol.one,this);
      else
        transmit(this->protocol.zero,this);
    }
    transmit(this->protocol.syncFactor,this);
  }

  // Disable transmit after sending (i.e., for inverted protocols)
  digitalWrite(this->nTransmitterPin, LOW);


  // enable receiver again if we just disabled it
  if (nReceiverInterrupt_backup != -1) {
    enableReceive(nReceiverInterrupt_backup,this);
  }

}

/**
 * Transmit a single high-low pulse.
 */
void transmit(HighLow pulses,Variables* this) {
  uint8_t firstLogicLevel = (this->protocol.invertedSignal) ? LOW : HIGH;
  uint8_t secondLogicLevel = (this->protocol.invertedSignal) ? HIGH : LOW;
  
  digitalWrite(this->nTransmitterPin, firstLogicLevel);
  ets_delay_us(this->protocol.pulseLength * pulses.high);
  digitalWrite(this->nTransmitterPin, secondLogicLevel);
  ets_delay_us(this->protocol.pulseLength * pulses.low);
}



/**
 * Enable receiving data
 */
esp_err_t enableReceive(int interrupt,Variables* this) {
  this->nReceiverInterrupt = interrupt;
  enableReceive1(this);
  return ESP_OK;
}

void enableReceive1(Variables* this) {
  if (this->nReceiverInterrupt != -1) {
    this->nReceivedValue = 0;
    this->nReceivedBitlength = 0;

   // attachInterrupt(this->nReceiverInterrupt, handleInterrupt, CHANGE);
    gpio_set_intr_type((gpio_num_t) (this->nReceiverInterrupt), GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)(this->nReceiverInterrupt),gpio_interrupt_handler, (void *)this);

  }
}

/**
 * Disable receiving data
 */
void disableReceive(Variables* this) {
// #if not defined(RaspberryPi) // Arduino
  //detachInterrupt(this->nReceiverInterrupt);
  gpio_isr_handler_remove((gpio_num_t) (this->nReceiverInterrupt));
  gpio_uninstall_isr_service();
// #endif // For Raspberry Pi (wiringPi) you can't unregister the ISR
  this->nReceiverInterrupt = -1;
}

bool available(Variables* this) {
  return this->nReceivedValue != 0;
}

void resetAvailable(Variables* this) {
 this->nReceivedValue = 0;
}

unsigned long getReceivedValue(Variables* this) {
  return this->nReceivedValue;
}

unsigned int getReceivedBitlength(Variables* this) {
  return this->nReceivedBitlength;
}

unsigned int getReceivedDelay(Variables* this) {
  return this->nReceivedDelay;
}

unsigned int getReceivedProtocol(Variables* this) {
  return this->nReceivedProtocol;
}

int* getReceivedRawdata(Variables* this) {
  return this->timings;
}

/* helper function for the receiveProtocol method */
static inline unsigned int diff(int A, int B) {
  return abs(A - B);
}

/**
 *
 */
bool RECEIVE_ATTR receiveProtocol(const int p, unsigned int changeCount,Variables* this) {
#if defined(ESP8266) || defined(ESP32)
    const Protocol &pro = proto[p-1];
#else
    Protocol pro;
    memcpy(&pro, &proto[p-1], sizeof(Protocol));


    unsigned long code = 0;
    //Assuming the longer pulse length is the pulse captured in timings[0]
    const unsigned int syncLengthInPulses =  ((pro.syncFactor.low) > (pro.syncFactor.high)) ? (pro.syncFactor.low) : (pro.syncFactor.high);
    const unsigned int delay = this->timings[0] / syncLengthInPulses;
    const unsigned int delayTolerance = delay * this->nReceiveTolerance / 100;
    
    /* For protocols that start low, the sync period looks like
     *               _________
     * _____________|         |XXXXXXXXXXXX|
     *
     * |--1st dur--|-2nd dur-|-Start data-|
     *
     * The 3rd saved duration starts the data.
     *
     * For protocols that start high, the sync period looks like
     *
     *  ______________
     * |              |____________|XXXXXXXXXXXXX|
     *
     * |-filtered out-|--1st dur--|--Start data--|
     *
     * The 2nd saved duration starts the data
     */
    const unsigned int firstDataTiming = (pro.invertedSignal) ? (2) : (1);

    for (unsigned int i = firstDataTiming; i < changeCount - 1; i += 2) {
        code <<= 1;
        if (diff(this->timings[i], delay * pro.zero.high) < delayTolerance &&
            diff(this->timings[i + 1], delay * pro.zero.low) < delayTolerance) {
            // zero
        } else if (diff(this->timings[i], delay * pro.one.high) < delayTolerance &&
                   diff(this->timings[i + 1], delay * pro.one.low) < delayTolerance) {
            // one
            code |= 1;
        } else {
            // Failed
            return false;
        }
    }

    if (changeCount > 7) {    // ignore very short transmissions: no device sends them, so this must be noise
       this->nReceivedValue = code;
       this->nReceivedBitlength = (changeCount - 1) / 2;
       this->nReceivedDelay = delay;
       this->nReceivedProtocol = p;
        return true;
    }

    return false;
}

void RECEIVE_ATTR handleInterrupt(Variables* this) {

  static unsigned int changeCount = 0;
  static unsigned long lastTime = 0;
  static unsigned int repeatCount = 0;

  const long time = micros();
  const unsigned int duration = time - lastTime;

  if (duration > this->nSeparationLimit) {
    // A long stretch without signal level change occurred. This could
    // be the gap between two transmission.
    if ((repeatCount==0) || (diff(duration, this->timings[0]) < 200)) {
      // This long signal is close in length to the long signal which
      // started the previously recorded timings; this suggests that
      // it may indeed by a a gap between two transmissions (we assume
      // here that a sender will send the signal multiple times,
      // with roughly the same gap between them).
      repeatCount++;
      if (repeatCount == 2) {
        for(unsigned int i = 1; i <= numProto; i++) {
          if (receiveProtocol(i, changeCount,this)) {
            // receive succeeded for protocol i
            break;
          }
        }
        repeatCount = 0;
      }
    }
    changeCount = 0;
  }
 
  // detect overflow
  if (changeCount >= RCSWITCH_MAX_CHANGES) {
    changeCount = 0;
    repeatCount = 0;
  }

  this->timings[changeCount++] = duration;
  lastTime = time;  
}
#endif


static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
  
  // RCSwitch *rcswitch=(RCSwitch*) args;
  Variables* obj = (Variables*)args; 
   handleInterrupt(obj);
}