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
 * in_out - input/output textual communication interface. 
 * Uses uart_drv module for data transfer.
 ******************************************************************************/

//******************************************************************************
// Includes
//******************************************************************************
#include <stdio.h>
#include <stdarg.h>     //varg

#include "pico/stdlib.h"

#include "uart_drv.h"

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// Global Variables
//******************************************************************************

/***************************************************************************//**
* @brief Init Input/Output module
*******************************************************************************/
void io_init(void)
{
    uart_drv_init();
}

/***************************************************************************//**
* @brief Check if there are input characters and copy them to buffer
* @param buff [out] pointer to buffer where the received characters are copied
* @param buff_max_len [in] the length in bytes (characters) of buff
* @return count of copied characters
*******************************************************************************/
int io_gets(char * buff, int buff_max_len)
{
    return uart_drv_getRx(buff, buff_max_len);
}

/***************************************************************************//**
* @brief Write text to output
* @param txt [in] string to write to output
*******************************************************************************/
void io_puts(const char * txt)
{
    uart_drv_puts(txt);
}

/***************************************************************************//**
* @brief Write formatted data from variable argument list to output.
* @param format [in] string that contains the format string (see printf)
* @return the number of characters sent to output
*******************************************************************************/
int io_printf(const char* format, ...)
{
    char temp_buff[128];

    va_list argp;
    va_start(argp, format);
    int res = vsnprintf(temp_buff, sizeof(temp_buff), format, argp);
    va_end(argp);

    uart_drv_puts(temp_buff);
    return res;
}

/***************************************************************************//**
* @brief Output a buffer of data in a hexadecimal 'dump' format
* @param ptr_buffer [in] - pointer to the buffer to display as dump
* @param len [in] - length of the buffer (in bytes)
* @param addr [in] - address offset of data to be displayed
*******************************************************************************/
void io_dump(const void * ptr_buffer, int len, unsigned long addr)
{
    int i, j;
    const uint8_t * ptr = (const uint8_t *) ptr_buffer;

    for(i = 0; i < len;)
    {
        io_printf("%08lx  ", addr);

        for(j = 0; j < (int) (addr % 16); j++)
        {
            io_puts(".. ");
        }

        for(; (j < 16) && (i < len); j++, i++)
        {
            io_printf("%02hhx ", *ptr);
            ptr++;
            addr++;
        }

        io_puts("\r\n");
    }
}
