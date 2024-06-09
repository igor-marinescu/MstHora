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
 * disp_max - the 7-segments display driver MAX7221.
 * Uses spi_drv module for the SPI communication.
 ******************************************************************************/
#ifndef DISP_MAX_H
#define DISP_MAX_H

//******************************************************************************
// Includes
//******************************************************************************
#include "ustime.h"
#include "datetime_utils.h"

//******************************************************************************
// Defines
//******************************************************************************

// Re-send data to display every DISPMAX_REFRESH_TIME [us]
#define DISPMAX_REFRESH_TIME   50000L

// Switch display pages (in case there are > 1 page) every DISPMAX_PAGE_TIME [us]
#define DISPMAX_PAGE_TIME   1000000L

#define DISP_INIT   dispmax_init
#define DISP_CLEAR  dispmax_clear
#define DISP_INT    dispmax_int
#define DISP_HEX    dispmax_hex
#define DISP_PUTS   dispmax_puts
#define DISP_PUTCH  dispmax_putch
#define DISP_DOT    dispmax_dot
#define DISP_TIME   dispmax_time
#define DISP_POLL   dispmax_poll
#define DISP_INTENS dispmax_intensity

//******************************************************************************
// Typedefs
//******************************************************************************

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init display
void dispmax_init(void);

// Clear display
void dispmax_clear(void);

// Display integer value in decimal format
void dispmax_int(int val);

// Display integer value in hexadecimal format
void dispmax_hex(unsigned int val, int tab);

// Display a text
void dispmax_puts(const char * txt);

// Display a symbol at a specific position
void dispmax_putch(int pos, const char ch);

// Display/clear a dot at a specific position
void dispmax_dot(int pos, bool val);

// Display RTC time
void dispmax_time(const datetime_t * ptr_datetime);

// Set display intensity (0...15)
void dispmax_intensity(int intensity);

// Display polling function. Must be called every program cycle
void dispmax_poll(const ustime_t sys_ustime);

//******************************************************************************
#endif /* DISP_MAX_H */
