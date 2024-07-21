cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

include(pico_extras_import_optional.cmake)

project(msthora C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

# Debug/Logging: modify the following definitions for include header, printf 
# and dump function of the Debug module
add_compile_definitions(DEBUG_INCLUDE="in_out.h")       # include        
add_compile_definitions(DEBUG_PRINTF=io_printf)         # function(__VA_ARGS__)
add_compile_definitions(DEBUG_DUMP=io_dump)             # function(buff, len, addr)

# Select modules with active define
#add_compile_definitions(I2C_DRV_DEBUG)
#add_compile_definitions(I2C_MEM_DEBUG)
#add_compile_definitions(I2C_RTC_DEBUG)
add_compile_definitions(DCF77_DEBUG)
#add_compile_definitions(I2C_BH1750_DEBUG)
#add_compile_definitions(I2C_MAN_DEBUG)
#add_compile_definitions(RTC_INTERN_DEBUG)
add_compile_definitions(MAIN_DEBUG)

add_executable(msthora)
target_sources(msthora PRIVATE 
        ustime.c ustime.h
        utils.c utils.h
        gpio_drv.c gpio_drv.h 
        uart_drv.c uart_drv.h 
        in_out.c in_out.h
        cli.c cli.h
        cli_func.c cli_func.h
        spi_drv.c spi_drv.h 
        i2c_drv.c i2c_drv.h
        i2c_mem.c i2c_mem.h
        test_mem.c test_mem.h
        datetime_utils.c datetime_utils.h
        i2c_rtc.c i2c_rtc.h
        i2c_bh1750.c i2c_bh1750.h
        i2c_manager.c i2c_manager.h
        dcf77.c dcf77.h
        rtc_intern.c rtc_intern.h
        main.c 
        )

# How to use options:
# Everytime when the option must be changed, invoke cmake with new option value:
# (the option value will be stored in cmake cache)
#       cmake . -DUSE_DISP_MAX=ON
# Make project:
#       make -j4
#
option(USE_DISP_MAX "Option to use display max" ON)
if(USE_DISP_MAX)
        target_sources(msthora PRIVATE disp_max.c disp_max.h)
        add_compile_definitions(DISP_INCLUDE="disp_max.h")
        message("USE_DISP_MAX: ON")
else()
        target_sources(msthora PRIVATE disp7seg.c disp7seg.h)
        add_compile_definitions(DISP_INCLUDE="disp7seg.h")
        message("USE_DISP_MAX: OFF")
endif()

# pull in common dependencies and additional uart hardware support
target_link_libraries(msthora 
        pico_stdlib 
        hardware_uart 
        hardware_spi 
        hardware_i2c 
        hardware_rtc
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(msthora)
