/*******************************************************************************
 * This file is part of the MstHora distribution.
 * Copyright (c) 2024 Igor Marinescu (igor.marinescu@gmail.com).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/
/*******************************************************************************
 * cli_func - Command Line Interface Functions
 * Defines all functions available in CLI
 ******************************************************************************/

//******************************************************************************
// Includes
//******************************************************************************
#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "cli_func.h"
#include "cli.h"
#include "utils.h"
#include "in_out.h"
#include "i2c_rtc.h"
#include "i2c_drv.h"
#include "i2c_manager.h"
#include "spi_drv.h"
#include "test_mem.h"
#include "i2c_bh1750.h"
#include "dcf77.h"
#include "rtc_intern.h"
#include DISP_INCLUDE

//******************************************************************************
// Function Prototypes
//******************************************************************************
bool cli_func_help(int argc, char ** args);
bool cli_func_memdump(int argc, char ** args);
bool cli_func_regdump(int argc, char ** args);
bool cli_func_dummy(int argc, char ** args);
bool cli_func_systime(int argc, char ** args);

bool cli_func_rtc_read(int argc, char ** args);
bool cli_func_rtc_set(int argc, char ** args);

bool cli_func_rtcint_read(int argc, char ** args);
bool cli_func_rtcint_set(int argc, char ** args);

bool cli_func_display(int argc, char ** args);
bool cli_func_test(int argc, char ** args);

bool cli_func_test_mem(int argc, char ** args);

bool cli_func_bh1750_init(int argc, char ** args);
bool cli_func_bh1750_read(int argc, char ** args);

bool cli_func_dcf77(int argc, char ** args);

bool cli_func_intens(int argc, char ** args);

//******************************************************************************
// Global Variables
//******************************************************************************
int cli_test_val1 = 0;
int cli_test_val2 = 0;
int cli_display = 0;
int cli_intens = -1;

/***************************************************************************//**
* @brief Init CLI functions
*******************************************************************************/
void cli_func_init(void)
{
    cli_add_func("?",        NULL,  cli_func_help,          "?");
    cli_add_func("help",     NULL,  cli_func_help,          "help");
    cli_add_func("memdump",  NULL,  cli_func_memdump,       "memdump <addr> <len>");
    cli_add_func("regdump",  NULL,  cli_func_regdump,       "regdump");
    cli_add_func("systime",  NULL,  cli_func_systime,       "systime");
    cli_add_func("dummy",    NULL,  cli_func_dummy,         "dummy");
    cli_add_func("rtc",    "read",  cli_func_rtc_read,      "rtc read");
    cli_add_func("rtc",     "set",  cli_func_rtc_set,       "rtc set <time> <date>");
    cli_add_func("rtcint", "read",  cli_func_rtcint_read,   "rtcint read");
    cli_add_func("rtcint",  "set",  cli_func_rtcint_set,    "rtcint set <time> <date>");
    cli_add_func("display",  NULL,  cli_func_display,       "display <val>");
    cli_add_func("test",     NULL,  cli_func_test,          "test <val>");
    cli_add_func("test_mem", NULL,  cli_func_test_mem,      "test_mem <op> <addr> <len> [pattern] [size_pattern]");
    cli_add_func("bh1750", "init",  cli_func_bh1750_init,   "bh1750 init");
    cli_add_func("bh1750", "read",  cli_func_bh1750_read,   "bh1750 read");
    cli_add_func("dcf77",    NULL,  cli_func_dcf77,         "dcf77");
    cli_add_func("intens",   NULL,  cli_func_intens,        "intens <value>");
}

/***************************************************************************//**
* @brief Display the list of all commands and their info
* @param argc [in] - arguments count
* @param args [in] - array with pointers to arguments string
* @return true if function successfully executed, false in case of error
*******************************************************************************/
bool cli_func_help(int argc, char ** args)
{
    cli_func_list();
    return true;
}

/***************************************************************************//**
* @brief Display a memdump (in hexadecimal format)
* @param argc [in] - arguments count
* @param args [in] - array with pointers to arguments string
* @return true if function successfully executed, false in case of error
*******************************************************************************/
bool cli_func_memdump(int argc, char ** args)
{
    long addr;
    int len;

    if(argc < 3)
        return false;

    if(!utils_get_long(&addr, args[1], CLI_WORD_SIZE))
        return false;

    if(!utils_get_int(&len, args[2], CLI_WORD_SIZE))
        return false;

    io_printf("memdump %0X %i:\r\n", addr, len);
    io_dump((unsigned char *) addr, len, addr);
    return true;
}

/***************************************************************************//**
* @brief Dummy function, displays the arguments count and their values
* @param argc [in] - arguments count
* @param args [in] - array with pointers to arguments string
* @return true if function successfully executed, false in case of error
*******************************************************************************/
bool cli_func_dummy(int argc, char ** args)
{
    io_printf("Hello from dummy. Arguments count=%i:\r\n", argc);

    int i;
    for(i = 0; i < argc; i++)
    {
        char * ptr = args[i];
        if((ptr != NULL) && (*ptr != '\0'))
            io_printf("Arg[%i] = >%s<\r\n", i, ptr);
        else
            io_printf("Arg[%i] -\r\n", i);
    }

    return true;
}

/***************************************************************************//**
* @brief Display registers
* @param argc [in] - arguments count
* @param args [in] - array with pointers to arguments string
* @return true if function successfully executed, false in case of error
*******************************************************************************/
void cli_display_reg(unsigned long reg_addr)
{
    uint8_t * ptr_reg = (uint8_t *) reg_addr;
    io_printf("reg[%#x]: %02x %02x %02x %02x\r\n", (unsigned long)ptr_reg, ptr_reg[0], ptr_reg[1], ptr_reg[2], ptr_reg[3]);
}

bool cli_func_regdump(int argc, char ** args)
{
    io_puts("regdump\r\n");
    cli_display_reg(0xe000e100);
    return true;
}

/***************************************************************************//**
* @brief Display system time in us
* @param argc [in] - arguments count
* @param args [in] - array with pointers to arguments string
* @return true if function successfully executed, false in case of error
*******************************************************************************/
bool cli_func_systime(int argc, char ** args)
{
    io_printf("systime: - function deleted\r\n");
    return false;
}

/***************************************************************************//**
* @brief RTC read
*           rtc read
*
* @param argc [in] - arguments count
* @param args [in] - array with pointers to arguments string
* @return true if function successfully executed, false in case of error
*******************************************************************************/
void cli_func_rtc_read_callback(int result)
{
    io_printf("cli_func_rtc_read_callback\r\n");
    i2c_err_t res = (i2c_err_t) result;
    if(res == i2c_success)
    {
        datetime_t * dt_ptr = i2c_rtc_get_datetime();
        if(dt_ptr != NULL)
        {
            io_puts("Ok\r\n");
            //io_printf("%2.2i:%2.2i:%2.2i  ", dt_ptr->hour, dt_ptr->min, dt_ptr->sec);
            //io_printf("%2.2i.%2.2i.%2.2i  ", dt_ptr->day, dt_ptr->month, dt_ptr->year);
            //io_printf("(%i)\r\n", dt_ptr->dotw);
            DATETIME_PRINTF_TIME(io_printf, "", *dt_ptr, " ");
            DATETIME_PRINTF_DATE(io_printf, "", *dt_ptr, "\r\n");
        }
        else{
            io_puts("Error: null\r\n");
        }
    }
    else {
        io_printf("Error: i2c_err=%i\r\n", (int) res);
    }
}

bool cli_func_rtc_read(int argc, char ** args)
{
    io_printf("cli_func_rtc_read\r\n");
    return i2c_man_req_rtc_read(cli_func_rtc_read_callback);
}

/***************************************************************************//**
* @brief RTC set
*          rtc set <time> <date> [day] 
*
* @param argc [in] - arguments count
* @param args [in] - array with pointers to arguments string
* @return true if function successfully executed, false in case of error
*******************************************************************************/
void cli_func_rtc_set_callback(int result)
{
    i2c_err_t res = (i2c_err_t) result;
    if(res == i2c_success)
        io_puts("Ok\r\n");
    else
        io_printf("Error: i2c_err=%i\r\n", (int) res);
}

bool cli_func_rtc_set(int argc, char ** args)
{
    if(argc < 4)
        return false;

    datetime_t dt;
    // Convert time string (arg[2]) to datetime variable
    if(datetime_time_from_text(&dt, args[2], CLI_WORD_SIZE) < 0)
    {
        io_puts("Error: wrong time format, expected: hh:mm:ss\r\n");
        return true;
    }

    // Convert date string (arg[3]) to datetime variable
    dt.dotw = 1;
    if(datetime_date_from_text(&dt, args[3], CLI_WORD_SIZE) < 0)
    {
        io_puts("Error: wrong date format, expected: DD.MM.YY\r\n");
        return true;
    }

    // Day also provided?
    if(argc >= 5)
    {
        int val;
        if(!utils_get_int(&val, args[4], CLI_WORD_SIZE) || (val < 1) || (val > 7))
        {
            io_puts("Error: wrong day format, must be 1..7\r\n");
            return true;
        }
        dt.dotw = (int8_t) val;
    }

    //io_printf("%2.2i:%2.2i:%2.2i  ", dt.hour, dt.min, dt.sec);
    //io_printf("%2.2i.%2.2i.%2.2i  ", dt.day, dt.month, dt.year);
    //io_printf("(%i)\r\n", dt.dotw);
    DATETIME_PRINTF_TIME(io_printf, "", dt, " ");
    DATETIME_PRINTF_DATE(io_printf, "", dt, "\r\n");

    return i2c_man_req_rtc_set(&dt, cli_func_rtc_set_callback);
}

/***************************************************************************//**
* @brief RTC-intern read
*           rtcint read
*
* @param argc [in] - arguments count
* @param args [in] - array with pointers to arguments string
* @return true if function successfully executed, false in case of error
*******************************************************************************/
bool cli_func_rtcint_read(int argc, char ** args)
{
    io_printf("cli_func_rtcint_read\r\n");

    datetime_t * dt_ptr = rtc_int_get_datetime();
    if(dt_ptr != NULL)
    {
        io_puts("Ok\r\n");
        //io_printf("%2.2i:%2.2i:%2.2i  ", dt_ptr->hour, dt_ptr->min, dt_ptr->sec);
        //io_printf("%2.2i.%2.2i.%2.2i  ", dt_ptr->day, dt_ptr->month, dt_ptr->year);
        //io_printf("(%i)\r\n", dt_ptr->dotw);
        DATETIME_PRINTF_TIME(io_printf, "", *dt_ptr, " ");
        DATETIME_PRINTF_DATE(io_printf, "", *dt_ptr, "\r\n");
    }
    else{
        io_puts("Error: null\r\n");
    }

    return true;
}

/***************************************************************************//**
* @brief RTC-intern set
*          rtcint set <time> <date> [day] 
*
* @param argc [in] - arguments count
* @param args [in] - array with pointers to arguments string
* @return true if function successfully executed, false in case of error
*******************************************************************************/
bool cli_func_rtcint_set(int argc, char ** args)
{
    if(argc < 4)
        return false;

    datetime_t dt;
    // Convert time string (arg[2]) to datetime variable
    if(datetime_time_from_text(&dt, args[2], CLI_WORD_SIZE) < 0)
    {
        io_puts("Error: wrong time format, expected: hh:mm:ss\r\n");
        return true;
    }

    // Convert date string (arg[3]) to datetime variable
    dt.dotw = 1;
    if(datetime_date_from_text(&dt, args[3], CLI_WORD_SIZE) < 0)
    {
        io_puts("Error: wrong date format, expected: DD.MM.YY\r\n");
        return true;
    }

    // Day also provided?
    if(argc >= 5)
    {
        int val;
        if(!utils_get_int(&val, args[4], CLI_WORD_SIZE) || (val < 1) || (val > 7))
        {
            io_puts("Error: wrong day format, must be 1..7\r\n");
            return true;
        }
        dt.dotw = (int8_t) val;
    }

    //io_printf("%2.2i:%2.2i:%2.2i  ", dt.hour, dt.min, dt.sec);
    //io_printf("%2.2i.%2.2i.%2.2i  ", dt.day, dt.month, dt.year);
    //io_printf("(%i)\r\n", dt.dotw);
    DATETIME_PRINTF_TIME(io_printf, "", dt, " ");
    DATETIME_PRINTF_DATE(io_printf, "", dt, "\r\n");

    if(rtc_int_set(&dt))
        io_puts("Ok\r\n");
    else
        io_printf("Error\r\n");

    return true;
}

/***************************************************************************//**
* @brief Set display mode
* @param argc [in] - arguments count
* @param args [in] - array with pointers to arguments string
* @return true if function successfully executed, false in case of error
*******************************************************************************/
bool cli_func_display(int argc, char ** args)
{
    if(argc < 2)
    {
        io_printf("display mode = %i\r\n", cli_display);
        return true;
    }

    if(!utils_get_int(&cli_display, args[1], CLI_WORD_SIZE))
        return false;

    io_printf("display mode set to %i\r\n", cli_display);
    return true;
}

/***************************************************************************//**
* @brief Test function
* @param argc [in] - arguments count
* @param args [in] - array with pointers to arguments string
* @return true if function successfully executed, false in case of error
*******************************************************************************/
bool cli_func_test(int argc, char ** args)
{
    if(argc < 2)
        return false;

    if(!utils_get_int(&cli_test_val1, args[1], CLI_WORD_SIZE))
        return false;

    cli_test_val2 = 0;
    if(argc > 2)
    {
        if(!utils_get_int(&cli_test_val2, args[2], CLI_WORD_SIZE))
            return false;
    }

    return true;
}

/***************************************************************************//**
* @brief Execute test_mem request:
*
*           args[0] | args[1] | args[2] | args[3] |    args[4]    |    args[5]
*           test_mem    write    <addr>    <len>    [data_pattern]  [size_pattern]
*           test_mem    read     <addr>    <len>    [data_pattern]  [size_pattern]
*           test_mem    check    <addr>    <len>    [data_pattern]  [size_pattern]
*           test_mem    auto
*
* @param argc [in] count of arguments in args array
* @param args [in] array of arguments, every element is a pointer to a string
* @return true - if the request successfully processed
*         false - error converting arguments to request
*******************************************************************************/
void cli_func_test_mem_callback(int result)
{
    if(result)
        io_puts("Memory test success\r\n");
    else 
        io_puts("Memory test error\r\n");
}

bool cli_func_test_mem(int argc, char ** args)
{
    test_mem_req_t req;

    int addr;   // args[2]
    int len;    // args[3]
    int data_pattern = (int) test_mem_data_pattern_zero;    // args[4]
    int size_pattern = (int) test_mem_size_pattern_max;     // args[5]

    memset(&req, 0, sizeof(req));

    // Set operation
    if(argc < 2)
        return false;

    if(strcmp(args[1], "write") == 0)
        req.op = test_mem_op_write;
    else if(strcmp(args[1], "read") == 0)
        req.op = test_mem_op_read;
    else if(strcmp(args[1], "check") == 0)
        req.op = test_mem_op_check;
    else if(strcmp(args[1], "auto") == 0)
        req.op = test_mem_op_auto;
    else
        return false;

    // Set request arguments (auto requires no arguments)
    if(req.op != test_mem_op_auto)
    {
        if(!utils_get_int(&addr, args[2], CLI_WORD_SIZE))
            return false;

        if(!utils_get_int(&len, args[3], CLI_WORD_SIZE))
            return false;

        if(argc >= 5)
            utils_get_int(&data_pattern, args[4], CLI_WORD_SIZE);

        if(argc >= 6)
            utils_get_int(&size_pattern, args[5], CLI_WORD_SIZE);

        req.addr = addr;
        req.len = len;
        req.data_pattern = (test_mem_data_pattern_t) data_pattern;
        req.size_pattern = (test_mem_size_pattern_t) size_pattern;
    }

    io_printf("cli_func_test_mem: req.op=%i\r\n", (int) req.op);
    return i2c_man_req_mem_test(&req, cli_func_test_mem_callback);
}

/***************************************************************************//**
* @brief Execute BH1750 init request:
*
*           args[0] | args[1]
*           bh1750    init
*
* @param argc [in] count of arguments in args array
* @param args [in] array of arguments, every element is a pointer to a string
* @return true - if the request successfully processed
*         false - error converting arguments to request
*******************************************************************************/
void cli_func_bh1750_init_callback(int result)
{
    i2c_err_t res = (i2c_err_t) result;
    if(res == i2c_success)
        io_puts("BH1750 init success\r\n");
    else 
        io_puts("BH1750 init error\r\n");
}

bool cli_func_bh1750_init(int argc, char ** args)
{
    io_printf("cli_func_bh1750_init\r\n");
    return i2c_man_req_bh1750_init(cli_func_bh1750_init_callback);
}

/***************************************************************************//**
* @brief Execute BH1750 read request:
*
*           args[0] | args[1]
*           bh1750    read
*
* @param argc [in] count of arguments in args array
* @param args [in] array of arguments, every element is a pointer to a string
* @return true - if the request successfully processed
*         false - error converting arguments to request
*******************************************************************************/
void cli_func_bh1750_read_callback(int result)
{
    i2c_err_t res = (i2c_err_t) result;
    if(res == i2c_success)
        io_printf("BH1750 val=%i\r\n", i2c_bh1750_get_val());
    else 
        io_puts("BH1750 read error\r\n");
}

bool cli_func_bh1750_read(int argc, char ** args)
{
    io_printf("cli_func_bh1750_read\r\n");
    return i2c_man_req_bh1750_read(cli_func_bh1750_read_callback);
}

/***************************************************************************//**
* @brief Get dcf77 info:
*
* @param argc [in] count of arguments in args array
* @param args [in] array of arguments, every element is a pointer to a string
* @return true - if the request successfully processed
*         false - error converting arguments to request
*******************************************************************************/
bool cli_func_dcf77(int argc, char ** args)
{
    io_printf("dcf77 quality: %i\r\n", dcf_get_quality());
    return true;
}

/***************************************************************************//**
* @brief Override intensity
*
*           args[0] | args[1]
*           intens    <val>
*
*       Where <val> is the intensity to be used: 0 (min) ... 15 (max)
*       Special cases: -1 : stop overriding intenisity
*                     255 : special cases with minimal intensity and all 8 digits active
*
* @param argc [in] count of arguments in args array
* @param args [in] array of arguments, every element is a pointer to a string
* @return true - if the request successfully processed
*         false - error converting arguments to request
*******************************************************************************/
bool cli_func_intens(int argc, char ** args)
{
    if(argc < 2)
    {
        io_printf("intensity = %i\r\n", cli_intens);
        return true;
    }

    if(!utils_get_int(&cli_intens, args[1], CLI_WORD_SIZE))
        return false;

    DISP_INTENS(cli_intens);
    io_printf("intensity override to %i\r\n", cli_intens);
    return true;
}
