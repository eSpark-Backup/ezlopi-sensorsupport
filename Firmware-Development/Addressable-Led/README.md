# Testing Addressable LED Strip using RMT controller with Serial JSON Commands


## How to use 
- Connect LED Strip Data pin to GPIO 16
    - Configurable through menuconfig ```idf.py menuconfig```
- Connect LED Strip GND to Board's GND
- Upload the firmware using `idf.py -p PORT flash`
- Send JSON Packet using Serial Monitor
     - Arduino Serial Recommended
- Or use Web Application in test Directory

## Web Application
![image](https://github.com/eSpark-Consultants/ezlopi-sensorsupport/assets/143803999/19fc0d04-077c-4a91-9f37-b94e67456250)

## Example JSON Packets:

### 1. To Change Effects (1D and 2D)

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
Change is "fx" value to change animations on the led strip (0-66)\
:red_circle: NOTE: From UI it will be a drop down list of all Effects.

### 2. To Change RGB Color Values (1D and 2D)

```
{
  "seg": {
    "col": [
      [
        180,
        180,
        0,
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
        0,
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

### 3. To Change Brightness of the Strip (1D and 2D)

```
{
  "bri": 50,
  "v": true,
  "time": 1697181526
}
```
Change the "bri": X value to control strip brightness (0-255)

### 4. To Select/Deselect Segment (1D and 2D)

> Select

```
{
  "seg": {
    "id": 0,
    "n": "name",
    "sel": true
  },
  "v": true,
  "time": 1697725589
}
```

> Deselect

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

### 5. To Change Segment Start and End Pixels (same is used to create a segment) (1D)

```
{
  "seg": {
    "id": 0,
    "n": "name",
    "start": 0,
    "stop": 30
  },
  "v": true,
  "time": 1697725589
}
```
```
{
  "seg": {
    "id": 0,
    "n": "name",
    "start": 10,
    "stop": 50
  },
  "v": true,
  "time": 1697725589
}
```

Change the "start": x and "stop":x to modify segment length.

### 6. To Change Selected Effects Speed (1D and 2D)

```
{
  "seg": {
    "sx": 255
  },
  "v": true,
  "time": 1698672533
}
```
Change "sx" : x value to change the speed of all selected effects (0-255)


### 7. To Change Selected Effects Intensity (1D and 2D)

```
{
  "seg": {
    "ix": 255
  },
  "v": true,
  "time": 1698672533
}
```
Change "ix" : x value to change the intensity of all selected effects (0-255)

### 8. To Change the Direction of the Effect (1D)

```
{
  "seg": {
    "id": 0,
    "rev": true
  },
  "v": true,
  "time": 1698747322
}
```
Change "rev" : x value to change the direction of effect of segment of id "id" : y. (true/false)

### 9. To Delete Segment using segment ID (1D and 2D)

```
{
  "seg": {
    "id": 1,
    "stop": 0
  },
  "v": true,
  "time": 1698766318
}
```
Give Segment ID "id":x (int) to delete not the segment index.\
:red_circle: NOTE: Disable the delete button if there is only 1 segment left.

### 10. To apply a Palette on Selected Segments if supported (1D and 2D)

```
{
  "seg": {
    "pal": 27
  },
  "v": true,
  "time": 1698935917
}
```
Change "pal":x (int) to apply palette on segments which are selected.\
:red_circle: NOTE: From UI it will be a drop down list of all palettes.

### 11. To Change Start Stop of a 2D Strip (2D)

```
{
  "seg": {
    "id": 0,
    "n": "",
    "start": 0,
    "stop": 10,
    "startY": 0,
    "stopY": 10,
    "grp": 1,
    "spc": 0,
    "of": 0,
    "tp": false
  },
  "v": true,
  "time": 1700472731
}
```



Select the instructions depending on Espressif chip installed on your development board:

- [ESP32 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html)
- [ESP32-S2 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)

## Progress
- [X] Effect Control using JSON
- [x] Primary Color Control using JSON
- [x] Secondary Color Control using JSON
- [x] Tertiary Color Control using JSON
- [x] Brightness Control using JSON
- [x] Segment Length Control
- [x] Add Multiple Segments 
- [x] Dynamic Segment Allocation
- [x] Effect Intensity Control
- [x] Effect Speed Control
- [x] Effect Reverse
- [x] Delete Segements
- [x] Understand Palettes
- [x] Add Palettes
- [x] Control Palettes using JSON
- [x] Understanding 2D Animation
- [ ] Configuration for 2D Led Strip
- [ ] Add 2d Animations Support for Led Matrix

:white_circle: Add Effects from WLED (in-progress)


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
├── testApp
│   ├── ARGB_TESTER_WEBAPP.html
│   └── images
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

