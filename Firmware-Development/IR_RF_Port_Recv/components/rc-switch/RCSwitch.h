/*
  RCSwitch - Arduino libary for remote control outlet switches
  Copyright (c) 2011 Suat Özgür.  All right reserved.

  Contributors:
  - Andre Koehler / info(at)tomate-online(dot)de
  - Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
  - Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=46
  - Dominik Fischer / dom_fischer(at)web(dot)de
  - Frank Oltmanns / <first name>.<last name>(at)gmail(dot)com
  - Max Horn / max(at)quendi(dot)de
  - Robert ter Vehn / <first name>.<last name>(at)gmail(dot)com
  
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
#ifndef _RCSwitch_h
#define _RCSwitch_h
#include "driver/gpio.h"
#include "esp_timer.h"
#include <stdint.h>
#include "esp_err.h"

static inline esp_err_t digitalWrite(unsigned char pin, bool state){
	return gpio_set_level((gpio_num_t)pin, state);
}

static inline esp_err_t pinMode(unsigned char pin, gpio_mode_t mode)
{
  return gpio_set_direction((gpio_num_t)pin, mode);
}

static inline int digitalRead(unsigned char  pin){
	return gpio_get_level((gpio_num_t)pin);
}

static inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static inline bool bitRead(uint8_t x,uint8_t y ){
    return ((x >> y) & 0x01);
}
typedef struct HighLow {
  uint8_t high;
  uint8_t low;
  }HighLow;

#define micros() esp_timer_get_time()
// Number of maximum high/Low changes per packet.
// We can handle up to (unsigned long) => 32 bit * 2 H/L changes per bit + 2 for sync
#define RCSWITCH_MAX_CHANGES 67
 /**
     * A "protocol" describes how zero and one bits are encoded into high/low
     * pulses.
     */
    typedef struct Protocol {
        /** base pulse length in microseconds, e.g. 350 */
        uint16_t pulseLength;

        HighLow syncFactor;
        HighLow zero;
        HighLow one;

        /**
         * If true, interchange high and low logic levels in all transmissions.
         *
         * By default, RCSwitch assumes that any signals it sends or receives
         * can be broken down into pulses which start with a high signal level,
         * followed by a a low signal level. This is e.g. the case for the
         * popular PT 2260 encoder chip, and thus many switches out there.
         *
         * But some devices do it the other way around, and start with a low
         * signal level, followed by a high signal level, e.g. the HT6P20B. To
         * accommodate this, one can set invertedSignal to true, which causes
         * RCSwitch to change how it interprets any HighLow struct FOO: It will
         * then assume transmissions start with a low signal lasting
         * FOO.high*pulseLength microseconds, followed by a high signal lasting
         * FOO.low*pulseLength microseconds.
         */
        bool invertedSignal;
    }Protocol;

typedef struct {
  int nReceiverInterrupt;
  int nTransmitterPin;
  int nRepeatTransmit;
  int nReceiveTolerance;
  unsigned long nReceivedValue;
  unsigned int nReceivedBitlength;
  unsigned int nReceivedDelay;
  unsigned int nReceivedProtocol;
   int nSeparationLimit;
  Protocol protocol;
  /* 
  * timings[0] contains sync timing, followed by a number of bits
  */
   int timings[RCSWITCH_MAX_CHANGES];
  }Variables;
// class RCSwitch {

//   public:
    void  RCSwitchInit(Variables* );
    void handleInterrupt(Variables* );
    void switchOnAdd(int nGroupNumber, int nSwitchNumber,Variables* this);
    void switchOffAdd(int nGroupNumber, int nSwitchNumber,Variables* this);
    void switchOn(const char* sGroup, int nSwitchNumber,Variables* this);
    void switchOff(const char* sGroup, int nSwitchNumber,Variables* this);
    void switchOnF(char sFamily, int nGroup, int nDevice,Variables* this);
    void switchOffF(char sFamily, int nGroup, int nDevice,Variables* this);
    void switchOnA(const char* sGroup, const char* sDevice,Variables* this);
    void switchOffA(const char* sGroup, const char* sDevice,Variables* this);
    void switchOnD(char sGroup, int nDevice,Variables* this);
    void switchOffD(char sGroup, int nDevice,Variables* this);

    void sendTriState(const char* sCodeWord,Variables* this);
    void sendC(unsigned long code, unsigned int length,Variables* this);
    void sendB(const char* sCodeWord,Variables* this);
    
    
    esp_err_t enableReceive(int interrupt,Variables* this);
    void enableReceive1(Variables* );
    void disableReceive(Variables* );
    bool available(Variables* this);
    void resetAvailable(Variables* this);

    unsigned long getReceivedValue(Variables* this);
    unsigned int getReceivedBitlength();
    unsigned int getReceivedDelay();
    unsigned int getReceivedProtocol();
    int* getReceivedRawdata();
   
  
    void enableTransmit(int nTransmitterPin,Variables* this);
    void disableTransmit();
    void setPulseLength(int nPulseLength,Variables* this);
    void setRepeatTransmit(int nRepeatTransmit,Variables* this);
    
    void setReceiveTolerance(int nPercent,Variables*);
    

    /**
     * Description of a single pule, which consists of a high signal
     * whose duration is "high" times the base pulse length, followed
     * by a low signal lasting "low" times the base pulse length.
     * Thus, the pulse overall lasts (high+low)*pulseLength
     */
 

   

    void setProtocol(Protocol protocol,Variables*);
    void setProtocolInt(int nProtocol,Variables*);
    void setProtocolPulse(int nProtocol, int nPulseLength,Variables*);


    char* getCodeWordA(const char* sGroup, const char* sDevice, bool bStatus);
    char* getCodeWordB(int nGroupNumber, int nSwitchNumber, bool bStatus);
    char* getCodeWordC(char sFamily, int nGroup, int nDevice, bool bStatus);
    char* getCodeWordD(char group, int nDevice, bool bStatus);
    void transmit(HighLow pulses,Variables* );

  
    
     bool receiveProtocol(const int p, unsigned int changeCount,Variables*);
  
    



    
// };

#endif
// extern RCSwitch rcswitch;
