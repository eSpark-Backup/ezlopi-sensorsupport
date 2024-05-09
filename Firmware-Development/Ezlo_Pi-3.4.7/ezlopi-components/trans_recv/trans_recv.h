#ifndef TRANS_RECV
#define TRANS_RECV

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include "ezlopi_hal_spi_master.h"
#include "esp_sleep.h"

#define ON_SYNC_WORD_BYTE 211
#define WAKE_UP_SYNC_WORD_BYTE 212
#define SHUTDOWN_SYNC_WORD_BYTE 213
#define NUM_TRIGGERS 3
#define Shutdown_GPIO GPIO_NUM_4

typedef struct
{
    uint32_t unique_id;
    uint32_t ON_SYNC_WORD;
    uint32_t WAKE_UP_SYNC_WORD;
    uint32_t SHUTDOWN_SYNC_WORD;
}Sync_Words_t;

enum TRIGGER
{
    POWERUP,
    SLEEP,
    SHUTDOWN
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

void trans_recv_init(enum TRANSCEIVER_MODE mode, s_ezlopi_spi_master_t *transReceive, cc1101_gpios_t *cc1101_gpios, Sync_Words_t *Sync_Words);
void apply_trigger(uint8_t trigger);

#endif //TRANS_RECV