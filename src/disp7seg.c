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

//******************************************************************************
// Includes
//******************************************************************************
#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "disp7seg.h"
#include "spi_drv.h"

//******************************************************************************
// Defines
//******************************************************************************
#define DISP7SEG_DOT_SET_INV    0xBF    // ~0x40 (Dot set - value inverted)
#define DISP7SEG_DOT_CLR_INV    0xFF    // ~0x00 (Dot clear - value inverted)

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// Global Variables
//******************************************************************************
const uint8_t disp7seg_tab[] = {
    //00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F  //      | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B | C | D | E | F |
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 00   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
    0x80, 0x10, 0x20, 0x02, 0x04, 0x08, 0x01, 0x82, 0x14, 0x28, 0x30, 0x15, 0x01, 0x29, 0x0C, 0x00, // 10   |sgA|sgB|sgC|sgD|sgE|sgF|sgG|sAD|sBE|sCF|sBC|BGE|sgG|CFG|sEF|   | 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x99, 0x01, 0x40, 0x15, // 20   |   | ! | " | # | $ | % | & | ' | ( | ) | * | + | , | - | . |'/'|
    0xBE, 0x30, 0x97, 0xB3, 0x39, 0xAB, 0xAF, 0xB0, 0xBF, 0xBB, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, // 30   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | : | ; | < | = | > | ? |
    0x00, 0xBD, 0x2F, 0x8E, 0x37, 0x8F, 0x8D, 0xAE, 0x3D, 0x30, 0x36, 0x00, 0x0E, 0xBC, 0x25, 0x27, // 40   | @ | A | B | C | D | E | F | G | H | I | J | K | L | M | N | O |
    0x9D, 0x00, 0x05, 0xAB, 0x0F, 0x3E, 0x00, 0x00, 0x00, 0x3B, 0x97, 0x8E, 0x29, 0xB2, 0x00, 0x02, // 50   | P | Q | R | S | T | U | V | W | X | Y | Z | [ | \ | ] | ^ | _ |
    0x00, 0xBD, 0x2F, 0x8E, 0x37, 0x8F, 0x8D, 0xAE, 0x3D, 0x30, 0x36, 0x00, 0x0E, 0xBC, 0x25, 0x27, // 60   | ` | a | b | c | d | e | f | g | h | i | j | k | l | m | n | o |
    0x9D, 0x00, 0x05, 0xAB, 0x0F, 0x3E, 0x00, 0x00, 0x00, 0x3B, 0x97, 0x8E, 0x29, 0xB2, 0x00, 0x00  // 70   | p | q | r | s | t | u | v | w | x | y | z | { | | | } | ~ |   |
};

const int disp7seg_tab_len = sizeof(disp7seg_tab);

static char frame_buffer[8];
static uint8_t raw_buffer[4];
static uint8_t dot_buffer[4];

static ustime_t sys_ustime_old = 0L;    // Tracking time to control display refresh
static ustime_t page_sw_ustime = 0L;    // Page switch time (in us)

static bool page_disp_2nd = false;      // Flag indicates the 2nd page is displayed

/***************************************************************************//**
* @brief Clear display
*******************************************************************************/
void disp7seg_clear(void)
{
    memset(frame_buffer, 0, sizeof(frame_buffer));
    memset(dot_buffer, DISP7SEG_DOT_CLR_INV, sizeof(dot_buffer));
}

/***************************************************************************//**
* @brief Init display
*******************************************************************************/
void disp7seg_init(void)
{
    disp7seg_clear();
}

/***************************************************************************//**
* @brief Display integer value in decimal format
* @param val [in] Integer value to be displayed 
*******************************************************************************/
void disp7seg_int(int val)
{
    int idx = sizeof(frame_buffer) - 1;
    bool negative = (val < 0);
    bool trim = false;

    if(negative)
    {
        val = -val;
        trim = (val >= 1000);   // Displayed in two pages as: [__-1][_000]
    }
    else{
        trim = (val >= 10000);  // Displayed in two pages as: [__10][_000]
    }

    frame_buffer[idx] = '0';

    while((val != 0) && (idx >= 0))
    {
        if(trim && (idx == (sizeof(frame_buffer) - 4)))
            frame_buffer[idx] = ' ';
        else{
            frame_buffer[idx] = '0' + (char)(val % 10);
            val /= 10;
        }
        idx--;
    }

    if(negative && (idx > 0))
        frame_buffer[idx] = '-';
}

/***************************************************************************//**
* @brief Display integer value in hexadecimal format
* @param val [in] Integer value to be displayed
* @param tab [in] The width of the displayed value (filled with zero)
*******************************************************************************/
void disp7seg_hex(unsigned int val, int tab)
{
    int idx = sizeof(frame_buffer) - 1;
    unsigned int digit;

    frame_buffer[idx] = '0';

    while((idx >= 0) && ((val != 0) || (tab > 0)))
    {
        digit = val & 0xF;
        if(digit < 10)
            frame_buffer[idx] = '0' + (char)(digit);
        else
            frame_buffer[idx] = 'A' + (char)(digit - 10);
        val >>= 4;
        idx--;
        tab--;
    }
}

/***************************************************************************//**
* @brief Display a text
* @param txt [in] Text to be displayed
*******************************************************************************/
void disp7seg_puts(const char * txt)
{
    const char * txt_end = txt;
    int idx = sizeof(frame_buffer) - 1;

    if((txt == NULL) || (*txt == '\0'))
        return;

    // Find the end of the string
    while(*txt_end != '\0') txt_end++;
    txt_end--;

    // Copy the letters backwards
    while((idx >= 0) && (txt_end >= txt))
    {
        frame_buffer[idx] = *txt_end;
        idx--;
        txt_end--;
    }
}

/***************************************************************************//**
* @brief Display a symbol at a specific position
* @param pos [in] position (7,6,5...1,0) where to display the symbol
* @param ch [in] symbol to be displayed
*******************************************************************************/
void disp7seg_putch(int pos, const char ch)
{
    if((pos < 0) || (pos >= sizeof(frame_buffer)))
        return;

    frame_buffer[pos++] = ch;

    // If there are any empty ('\0') characters until the end of framebuffer
    // fill them with spaces (' ')
    while(pos < sizeof(frame_buffer))
    {
        if(frame_buffer[pos] == '\0')
            frame_buffer[pos] = ' ';
        pos++;
    }
}

/***************************************************************************//**
* @brief Display/clear a dot at a specific position
* @param pos [in] position (display index 3,2,1,0) where to display/clear the dot
* @param val [in] value of the dot
*                   [].[].[].[].
*             Dot:  3  2  1  0
*******************************************************************************/
void disp7seg_dot(int pos, bool val)
{
    if((pos < 0) || (pos >= 4))
        return;

    dot_buffer[pos] = val ? DISP7SEG_DOT_SET_INV : DISP7SEG_DOT_CLR_INV;
}

/***************************************************************************//**
* @brief Display RTC time
* @param ptr_time [in] pointer to time structure to display
*******************************************************************************/
void disp7seg_time(const datetime_t * ptr_datetime)
{
    if(ptr_datetime->hour == 0)
    {
        disp7seg_puts("000");
        disp7seg_int(ptr_datetime->min);
    }
    else{
        disp7seg_int((ptr_datetime->hour * 100) + ptr_datetime->min);
    }    

}

/***************************************************************************//**
* @brief Set display intensity (0...15)
* @param intensity [in] display intensity (0..15)
*******************************************************************************/
void disp7seg_intensity(int intensity)
{
    (void) intensity;
    // Do nothing, dummy function, the display doesn't support intensity change
}

/***************************************************************************//**
* @brief Display polling function. Must be called every program cycle
*******************************************************************************/
void disp7seg_poll(const ustime_t sys_ustime)
{
    uint8_t digit;
    int idx, frame_off;

    if(get_diff_ustime(sys_ustime, sys_ustime_old) < DISP7SEG_REFRESH_TIME)
        return;

    sys_ustime_old = sys_ustime;

    if(spi_drv_is_busy())
        return;

    // Are there 2 pages to display? 
    if(frame_buffer[3] != 0)
    {
        // Display every page DISP7SEG_PAGE_TIME [us] long
        if(get_diff_ustime(sys_ustime, page_sw_ustime) > DISP7SEG_PAGE_TIME)
        {
            // Switch the page
            page_disp_2nd = !page_disp_2nd;
            page_sw_ustime = sys_ustime;
        }
    }
    else{
        // Display only 1st page [____][XXXX]
        page_disp_2nd = false;
    }

    // Set framebuffer offset based on page to display
    frame_off = (page_disp_2nd) ? 4 : 8;

    // Convert frame_buffer to display raw_data
    for(idx = 0; idx < 4; idx++)
    {
        digit = (uint8_t) frame_buffer[frame_off - idx - 1];
        raw_buffer[idx] = (digit < disp7seg_tab_len) ? ~disp7seg_tab[digit] : 0xFF;
        raw_buffer[idx] &= dot_buffer[idx];
    }

    spi_drv_send(raw_buffer, 4);
}
