#ifndef TRANS_RECV
#define TRANS_RECV

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "esp_sleep.h"

#define ON_SYNC_WORD_BYTE 211
#define WAKE_UP_SYNC_WORD_BYTE 212
#define SHUTDOWN_SYNC_WORD_BYTE 213
#define NUM_TRIGGERS 3


enum TRIGGER
{
    SHUTDOWN,
    POWERUP,
    SLEEP
};

enum TRANSCEIVER_MODE
{
    TRANSMITTER,
    RECIEVER
};

typedef struct
{
    const char *methodstr;
    uint8_t method;
}Triggers_t;


void trans_recv_init(enum TRANSCEIVER_MODE mode);


#endif //TRANS_RECV