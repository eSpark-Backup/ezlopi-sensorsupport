#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "trans_recv.h"


void app_main(void)
{
    trans_recv_init(TRANSMITTER);    //com13/
    // trans_recv_init(RECIEVER);          //com12 or com4
}