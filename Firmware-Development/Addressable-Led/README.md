# Testing Addressable LED Strip using RMT controller with Serial JSON Commands


## How to use 
- Connect LED Strip Data pin to GPIO 16
- Connect LED Strip GND to Board's GND
- Upload the firmware using `idf.py -p PORT flash`
- Send JSON Packet using Serial Monitor


- Example Json:

```
{
  "seg": {
    "fx": 31,
    "fxdef": true
  },
  "v": true,
  "time": 1697181639
}
```

- Change is "fx" value to change animations on the led strip (0-66)

Select the instructions depending on Espressif chip installed on your development board:

- [ESP32 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html)
- [ESP32-S2 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)


## Project folder contents

The project **Addressable-Led** contains main directory containing source files in C language [hello_world_main.c](main/hello_world_main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt` files that provide set of directives and instructions describing the project's source files and targets (executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt           
├── main
│   ├── CMakeLists.txt
│   ├── component.mk           Component make file
│   ├── hello_world_main.c
│   ├── config.c
│   ├── gamma.c
│   ├── led.c
│   ├── ownled.c
│   ├── playlist.c
│   ├── status.c
│   ├── uart_reader.c
│   ├── ws2812fx.c
│   └── fastrmt.S
├── Makefile                   Makefile used by legacy GNU Make
├── README.md                  This is the file you are currently reading
├── sdkconfig
├── sdkconfig.ci
└── sdkconfig.old
```

For more information on structure and contents of ESP-IDF projects, please refer to Section [Build System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html) of the ESP-IDF Programming Guide.

## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

