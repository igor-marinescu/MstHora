
# Project name

MstHora (Meister Hora, Secundus Minutius Hora)

Digital clock - based on Raspberry Pi-Pico - can be synchronized wirelessly - time recording even when the device is switched off - adjustment of the display intensity to the environment

# How to build:

This is a test project for pico using cmake.

Copy both folders test1_src and test1_build to linux virtual machine:

```
<path>--+
        +-[test1_build]
        |
        +-[test1_src]
```

Goto test1_build folder:

    $ cd <path>/test1_build
    
Export the path where [pico-sdk] folder is located.

By example, if pico-sdk is located here: /home/igor/pico/pico-sdk
   
    $ export PICO_SDK_PATH=/home/igor/pico/pico-sdk
   
Configure the project cmake:

    $ cmake ../test1_src
    
Build the project:

    $ make -j4

# Light sensor BH1750

Pinout:

```
VCC = 3.3V
GND = GND
SCL
SDA
ADDR = GND (0x23), VCC (0x5C)
```

# Display (Shift Registers)

```
spisend 0xFFFFFFFF	// ____  
spisend 0x41cf684c	// 1234  
spisend 0x42D071C8	// AbCd  
```

Display Connector

Connector-Pin | Display-Signal | Pico-Signal | Pico-PinIdx
--------------|----------------|-------------|------------
B1            | MISO-          | -           | -
A1            | VCC            | VSYS        | 39
B2            | GND            | GND         | 38
A2            | SLCK-          | SPI0 SCK    | 24
B3            | GND            | -           | -
A3            | RCK            | SPI0 CSn    | 22
B4            | GND            | -           | -
A4            | MOSI           | SPI0 TX     | 25
B5            | GND            | -           | -
A5            | /SS            | 3V3OUT      | 36
B6            | GND            | -           | -
A6            | IRQ            | -           | -

# Display (MAX Driver)

MAX7219/MAX7221
Serially Interfaced, 8-Digit LED Display Drivers

