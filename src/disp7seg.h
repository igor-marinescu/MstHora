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
 * disp7seg - the 7-segment display driver implemented as shift-registers.
 * Uses spi_drv module for the SPI communication.
 ******************************************************************************/
#ifndef DISP_7_SEG_H
#define DISP_7_SEG_H

//******************************************************************************
// Includes
//******************************************************************************
#include "ustime.h"
#include "datetime_utils.h"

//******************************************************************************
// Defines
//******************************************************************************

// Re-send data to display every DISP7SEG_REFRESH_TIME [us]
#define DISP7SEG_REFRESH_TIME   12000L

// Switch display pages (in case there are > 1 page) every DISP7SEG_PAGE_TIME [us]
#define DISP7SEG_PAGE_TIME  1000000L

#define DISP_INIT   disp7seg_init
#define DISP_CLEAR  disp7seg_clear
#define DISP_INT    disp7seg_int
#define DISP_HEX    disp7seg_hex
#define DISP_PUTS   disp7seg_puts
#define DISP_PUTCH  disp7seg_putch
#define DISP_DOT    disp7seg_dot
#define DISP_TIME   disp7seg_time
#define DISP_POLL   disp7seg_poll
#define DISP_INTENS disp7seg_intensity

//******************************************************************************
// Typedefs
//******************************************************************************

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init display
void disp7seg_init(void);

// Clear display
void disp7seg_clear(void);

// Display integer value in decimal format
void disp7seg_int(int val);

// Display integer value in hexadecimal format
void disp7seg_hex(unsigned int val, int tab);

// Display a text
void disp7seg_puts(const char * txt);

// Display a symbol at a specific position
void disp7seg_putch(int pos, const char ch);

// Display/clear a dot at a specific position
void disp7seg_dot(int pos, bool val);

// Display RTC time
void disp7seg_time(const datetime_t * ptr_datetime);

// Set display intensity (0...15)
void disp7seg_intensity(int intensity);

// Display polling function. Must be called every program cycle
void disp7seg_poll(const ustime_t sys_ustime);

//******************************************************************************
#endif /* DISP_7_SEG_H */
