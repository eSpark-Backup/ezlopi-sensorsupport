# RF-Shutdown-Wakeup-Sequence Port from Arduino to ESP-IDF

## Overview

Purpose of this branch is to port Arduino based IR_RF Firmware to ESP_IDF.

## Table of Contents

- [Getting Started](#getting-started)
- [Triggers](#Triggers)
- [Transceiver Modes](#Transceiver-Modes)
- [Prerequisites](#prerequisites)


## Getting Started

1. Download or Clone the project.
2. In main file initialize TRANSMITTER or RECEIVER
```
void app_main(void)
{
    trans_recv_init(TRANSMITTER);    
    // trans_recv_init(RECEIVER);        
}
```
3. Select the correct Port.
4. Upload firmware using ```idf.py -p COM12 flash ```
5. Power both TRANSMITTER and RECEIVER Devices.
6. Monitor both devices using any serial terminal
7. Send 1,2,3,4 to trigger sleep, wakeup, shutdown and powerup respectively


## Triggers

|   Character   |      Trigger       |   Enum   |
|:-------------:|:------------------:|:--------:|
|       1       |   Trigger Sleep    |  SLEEP   |
|       2       |  Trigger Wakeup    |    -     |
|       3       | Trigger Shutdown   | SHUTDOWN |
|       4       | Trigger Powerup    | POWERUP  |

## Transceiver Modes

```
enum TRANSCEIVER_MODE
{
    TRANSMITTER,
    RECIEVER
};
```


### Prerequisites

1. ESP-IDF v4.4
2. Voltlink or any USB to UART programmer


