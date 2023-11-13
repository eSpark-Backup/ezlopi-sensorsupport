# IR+RF Port from Arduino to ESP-IDF

## Overview

Purpose of this repo this to port Arduino based IR_RF Firmware to ESP_IDF

## Table of Contents

- [Getting Started](#getting-started)
- [Prerequisites](#prerequisites)


## Getting Started

1. Download or Clone the project.
2. For Reciever Side Comment out the trans_main.c from main/CMakeLists.txt
```
idf_component_register(SRCS 
                        "recv_main.c"
                        # "trans_main.c"
                INCLUDE_DIRS ".")
```
3. For Transmitter Side Comment out the recv_main.c from main/CMakeLists.txt
```
idf_component_register(SRCS 
                        # "recv_main.c"
                        "trans_main.c"
                INCLUDE_DIRS ".")
```
3. Select the correct Port.
3. Upload firmware using ```idf.py -p COM12 monitor```
3. Power both Rx and Tx Devices.

### Prerequisites

1. ESP-IDF v4.4
2. Voltlink or any USB to UART programmer


