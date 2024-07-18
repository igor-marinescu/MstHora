# Compile the Software and program MstHora

## Install the Toolchain and the Raspberry Pi Pico C/C++ SDK

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

## Clone MstHora repository and build the software

Clone the MstHora git repository:

```
$ git clone https://github.com/igor-marinescu/MstHora.git
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

## Load the Firmware onto MstHora

The build process generates `build/msthora.uf2` firmware file. To load the firmware onto MstHora board, mount it as a USB Mass Storage Device and copy rhe uf2 file onto the board to program the flash. Hold down the BOOTSEL button while plugging in your device using a micro-USB cable to force it into USB Mass Storage Mode. RP2040 will reboot, unmount itself as a Mass Storage Device, and run the flashed code.
