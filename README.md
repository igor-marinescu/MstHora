# Meister Hora: The DIY Digital Clock

Introducing Meister Hora (MstHora), a sophisticated and versatile digital clock designed for tech enthusiasts and DIY hobbyists. Powered by the robust Raspberry Pi Pico board, this clock not only shows time but does so with precision and adaptability, making it a standout addition to any room.

## Project Inspiration and Goals

The main goal of the MstHora project was to gather hands-on experience in designing and producing a product from the initial idea to the final functional piece. It embodies the journey of learning and creation, showcasing the blend of electronics, programming, and craftsmanship.

## Key Features

**DCF77 Radio Receiver**: MstHora leverages a DCF77 radio receiver to synchronize time and date with the highly accurate atomic clock signals broadcast from Mainflingen, Germany. This ensures that the clock is always precise, automatically adjusting for daylight saving time changes and other regional time adjustments.

**Real-Time Clock (RTC) Module**: Equipped with a reliable RTC module, MstHora maintains accurate timekeeping even when powered off. This backup feature ensures the clock's reliability, avoiding the need for manual resetting after power interruptions.

**Luminosity Sensor**: MstHora adapts to its environment with an integrated luminosity sensor. The sensor dynamically adjusts the intensity of the 7-segment display, providing optimal visibility whether it's day or night, bright or dim.

**7-Segment Display**: The clear and bright 7-segment 0.56" display makes it easy to read the time. The automatic brightness adjustment ensures that the display is never too harsh or too dim, preserving readability and eye comfort.

**Open-Source Design**: MstHora is entirely open-source, inviting everyone to explore, modify, and improve upon the design. This includes:

1. Software: Written in C, well documented, available for download.
2. Hardware: Detailed hardware schematics and component lists are provided, ensuring you have all the information needed to replicate or customize the project.
3. Mechanical Drawings: Comprehensive mechanical drawings are available, offering a clear blueprint for constructing the physical clock.

**Software**: The software driving MstHora is written in C, offering robust performance and efficient handling of all its functions. Key aspects include:
1. Communication Drivers: Custom-developed drivers handle communication between the Raspberry Pi Pico, the DCF77 receiver, the RTC module, the EEPROM memory and the luminosity sensor, ensuring seamless integration and operation. 
2. Decoding Algorithms: Sophisticated algorithm decodes the DCF77 time signal, converting it into accurate and reliable time data displayed on the 7-segment display.

**Hardware and Build**: MstHora is not just a digital watch, but an attempt to combine engineering and craftsmanship. The mechanical and electrical components are easy to find. No special tools or equipment are required to build this clock.

## Build MstHora

TODO!!!

## Compile the Software and program MstHora

### Install the Toolchain and the Raspberry Pi Pico C/C++ SDK

Follow the instructions described in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) to install the Toolchain and the SDK.

Here is a quick cheat sheet:

Create a pico directory at `/home/<user>`:

```
$ cd 
$ mkdir pico
$ cd pico
```

Clone the pico-sdk and pico-examples git repositories:

```
$ git clone https://github.com/raspberrypi/pico-sdk.git --branch master
$ cd pico-sdk
$ git submodule update --init
$ cd ..
$ git clone https://github.com/raspberrypi/pico-examples.git --branch master
```

Install the toolchain:

```
$ sudo apt update
$ sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

Updating the SDK (when a new version of the SDK is released):

```
$ cd pico-sdk
$ git pull
$ git submodule update
```

### Clone MstHora repository and build the software

Clone the MstHora git repository at `/home/<user>`:

```
$ cd 
$ git clone https://github.com/igor-marinescu/MstHora.git --branch master
$ cd MstHora
```

Export the path where pico-sdk directory is located:

```
$ export PICO_SDK_PATH=/home/<user>/pico/pico-sdk
```

Build the project:

```
$ cd build
$ cmake ../src
$ make -j4
```

### Load the Firmware onto MstHora

The build process generates `msthora.uf2` firmware file. To load the firmware onto MstHora board, mount it as a USB Mass Storage Device and copy a uf2 file onto the board to program the flash. Hold down the BOOTSEL button while plugging in your device using a micro-USB cable to force it into USB Mass Storage Mode. RP2040 will reboot, unmount itself as a Mass Storage Device, and run the flashed code.

## First Run

TODO!!!

## Test MstHora

TODO!!!

## Electrical Characteristics

Measurements:
- Current at minimal intensity: 27mA @ 5V (135mW)
- Current at maximal intensity: 45mA @ 5V (225mW)

## TODO:

- Explain on drawings the BOOTSEL button.
- Explain that the routing files are not really "routing", but instead for wire-connections designed.