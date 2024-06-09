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
#ifndef IN_OUT_H
#define IN_OUT_H

//******************************************************************************
// Includes
//******************************************************************************
#include "uart_drv.h"

//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Typedefs
//******************************************************************************

//******************************************************************************
// Exported Variables
//******************************************************************************

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init Input/Output module
void io_init(void);

// Check if there are input characters and copy them to buffer
int io_gets(char * buff, int buff_max_len);

// Write text to output
void io_puts(const char * txt);

// Write formatted data from variable argument list to output
int io_printf(const char* format, ...);

// Output a buffer of data in a hexadecimal 'dump' format
void io_dump(const void * ptr_buffer, int len, unsigned long addr);

//******************************************************************************
#endif /* IN_OUT_H */
