# Testing Addressable LED Strip using RMT controller with Serial JSON Commands


## How to use 
- Connect LED Strip Data pin to GPIO 16
    - Configurable through menuconfig ```idf.py menuconfig```
- Connect LED Strip GND to Board's GND
- Upload the firmware using `idf.py -p PORT flash`
- Send JSON Packet using Serial Monitor
     - Arduino Serial Recommended


## Example JSON Packets:

1. To Change Effects

```
{
  "seg": {
    "fx": 4,
    "fxdef": true
  },
  "v": true,
  "time": 1697181639
}
```
Change is "fx" value to change animations on the led strip (0-66)

2. To Change RGB Color Values

```
{
  "seg": {
    "col": [
      [
        180,
        180,
        255,
        0
      ],
      [
        0,
        180,
        255,
        0
      ],
      [
        180,
        180,
        255,
        0
      ]
    ]
  },
  "v": true,
  "time": 1697181431
}
```
Change the "col":[[r,g,b,w], [r,g,b,w], [r,g,b,w]] values to change RGB Colors on the led strip (0-255)

```
"col" : [[Primary], [Secondary], [Tertiary]] 
```

3. To Change Brightness of the Strip

```
{
  "bri": 210,
  "v": true,
  "time": 1697181526
}
```
Change the "bri": X value to control strip brightness (0-255)

4. To Select/Deselect Segment

```
{
  "seg": {
    "id": 0,
    "n": "name",
    "sel": false
  },
  "v": true,
  "time": 1697725589
}
```

Change the "sel": X bool value either true or false to control segment selection (true/flase)
any segment that is deselected will not be modifiable by any command

5. To Change Segment Start and End Pixels

```
{
  "seg": {
    "id": 1,
    "n": "name",
    "start": 0,
    "stop": 30
  },
  "v": true,
  "time": 1697725589
}
```

Change the "start": x and "stop":x to modify segment length.

Select the instructions depending on Espressif chip installed on your development board:

- [ESP32 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html)
- [ESP32-S2 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)

## TODOs
- [X] Effect Control using JSON
- [x] Primary Color Control using JSON
- [x] Secondary Color Control using JSON
- [x] Tertiary Color Control using JSON
- [x] Brightness Control using JSON
- [x] Segment Length Control
- [ ] Add Multiple Segments 
- [ ] Add Custom Effects from WLED


## Project folder contents

The project **Addressable-Led** contains main directory containing source files in C language [main.c](main/main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt` files that provide set of directives and instructions describing the project's source files and targets (executable, library, or both). 

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── components                Component directory
│   ├── config
│   │    ├── include
│   │    ├── src
│   │    └── CMakeLists.txt
│   ├── fastrmt
│   │    ├── include
│   │    ├── src
│   │    └── CMakeLists.txt
│   ├── led
│   │    ├── include
│   │    ├── src
│   │    └── CMakeLists.txt
│   ├── ownled
│   │    ├── include
│   │    ├── src
│   │    └── CMakeLists.txt
│   ├── playlist
│   │    ├── include
│   │    ├── src
│   │    └── CMakeLists.txt
│   ├── status
│   │    ├── include
│   │    ├── src
│   │    └── CMakeLists.txt
│   ├── uart_reader
│   │    ├── include
│   │    ├── src
│   │    └── CMakeLists.txt
│   └── ws2812fx
│        ├── include
│        ├── src
│        └── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   ├── component.mk           Component make file
│   ├── kconfig.projbuild
│   └── main.c
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

