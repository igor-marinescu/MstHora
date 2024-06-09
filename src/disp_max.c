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

//******************************************************************************
// Includes
//******************************************************************************
#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "disp_max.h"
#include "spi_drv.h"

//******************************************************************************
// Defines
//******************************************************************************
#define DISPMAX_DOT_SET     0x80    // Dot set
#define DISPMAX_DOT_CLR     0x00    // Dot clear

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// Global Variables
//******************************************************************************
static const uint8_t seg_tab[] = {
    //00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F  //      | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B | C | D | E | F |
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 00   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x48, 0x24, 0x12, 0x30, 0x25, 0x01, 0x13, 0x06, // 10   |dot|sgA|sgB|sgC|sgD|sgE|sgF|sgG|sAD|sBE|sCF|sBC|BGE|sgG|CFG|sEF|
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x80, 0x25, // 20   |   | ! | " | # | $ | % | & | ' | ( | ) | * | + | , | - | . |'/'|
    0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, // 30   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | : | ; | < | = | > | ? |
//          A     B     C     D    E     F      G     H     I    J      K    L     M     N     O
    0x00, 0x77, 0x7F, 0x4E, 0x7E, 0x4F, 0x47, 0x5E, 0x37, 0x30, 0x3C, 0x37, 0x0E, 0x76, 0x37, 0x7E, // 40   | @ | A | B | C | D | E | F | G | H | I | J | K | L | M | N | O |
    0x67, 0x7E, 0x77, 0x5B, 0x70, 0x3E, 0x3E, 0x3E, 0x37, 0x33, 0x6D, 0x4E, 0x13, 0x78, 0x62, 0x08, // 50   | P | Q | R | S | T | U | V | W | X | Y | Z | [ | \ | ] | ^ | _ |
    0x02, 0x7D, 0x1F, 0x0D, 0x3D, 0x6F, 0x47, 0x7B, 0x17, 0x0C, 0x18, 0x37, 0x0E, 0x15, 0x15, 0x1D, // 60   | ` | a | b | c | d | e | f | g | h | i | j | k | l | m | n | o |
    0x67, 0x73, 0x05, 0x5B, 0x0F, 0x1C, 0x1C, 0x1C, 0x37, 0x3B, 0x6D, 0x4E, 0x30, 0x78, 0x01, 0x00  // 70   | p | q | r | s | t | u | v | w | x | y | z | { | | | } | ~ |   |
};

static const int seg_tab_len = sizeof(seg_tab);

// Raw-data send to display
static uint8_t tx_data[16];
static int tx_idx = 0;
static int tx_cnt = 0;

static char frame_buffer[8];
static uint8_t dot_buffer[4];

static ustime_t sys_ustime_old = 0L;    // Tracking time to control display refresh
static ustime_t page_sw_ustime = 0L;    // Page switch time (in us)

static bool page_disp_2nd = false;      // Flag indicates the 2nd page is displayed

static uint8_t intensity_h = 0x08;      // Display intensity min=0, max=0x0F

/***************************************************************************//**
* @brief Prepare control registers to send
*******************************************************************************/
void prepare_control_tx(void)
{
    tx_data[0] = 0xFF;  // Display test off
    tx_data[1] = 0x00;
    tx_data[2] = 0xFC;  // Normal operation
    tx_data[3] = 0x0F;
    tx_data[4] = 0x09;  // Decode mode none
    tx_data[5] = 0x00;
    tx_data[6] = 0x0B;  // Scan limit: Digits3..0 or in special mode (255) Digits7..0
    tx_data[7] = (intensity_h == 255) ? 0x07 : 0x03;
    tx_data[8] = 0x0A;  // Intensity or in special mode (255) intenisty 0
    tx_data[9] = (intensity_h == 255) ? 0x00 : intensity_h;
    tx_idx = 0;
    tx_cnt = 10;
}

/***************************************************************************//**
* @brief Prepare digit registers to send
*******************************************************************************/
void prepare_digit_tx(void)
{
    int idx, i = 0;

    // Set framebuffer offset based on page to display
    int frame_off = (page_disp_2nd) ? 4 : 8;

    // Convert frame_buffer to display raw_data
    for(idx = 0; idx < 4; idx++)
    {
        // Digit index
        tx_data[i] = (uint8_t)(idx + 1);
        i++;
        // Digit value
        uint8_t digit = (uint8_t) frame_buffer[frame_off - idx - 1];
        tx_data[i] = (digit < seg_tab_len) ? seg_tab[digit] : 0x00;
        tx_data[i] |= dot_buffer[idx];
        i++;
    }

    // If intensity is in special mode (255) send all 8 digits
    if(intensity_h == 255)
    {
        // Fill the rest of data with FF
        for(idx = 4; idx < 8; idx++)
        {
            // Digit index
            tx_data[i] = (uint8_t)(idx + 1);
            i++;
            // Digit value
            tx_data[i] = 0xFF;
            i++;
        }
    }

    tx_idx = 0;
    tx_cnt = i;
}

/***************************************************************************//**
* @brief Clear display
*******************************************************************************/
void dispmax_clear(void)
{
    memset(frame_buffer, 0, sizeof(frame_buffer));
    memset(dot_buffer, DISPMAX_DOT_CLR, sizeof(dot_buffer));
}

/***************************************************************************//**
* @brief Init display
*******************************************************************************/
void dispmax_init(void)
{
    dispmax_clear();
    prepare_control_tx();
}

/***************************************************************************//**
* @brief Display integer value in decimal format
* @param val [in] Integer value to be displayed 
*******************************************************************************/
void dispmax_int(int val)
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
void dispmax_hex(unsigned int val, int tab)
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
void dispmax_puts(const char * txt)
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
void dispmax_putch(int pos, const char ch)
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
void dispmax_dot(int pos, bool val)
{
    if((pos < 0) || (pos >= 4))
        return;

    dot_buffer[pos] = val ? DISPMAX_DOT_SET : DISPMAX_DOT_CLR;
}

/***************************************************************************//**
* @brief Display RTC time
* @param ptr_time [in] pointer to time structure to display
*******************************************************************************/
void dispmax_time(const datetime_t * ptr_datetime)
{
    if(ptr_datetime->hour == 0)
    {
        dispmax_puts("000");
        dispmax_int(ptr_datetime->min);
    }
    else{
        dispmax_int((ptr_datetime->hour * 100) + ptr_datetime->min);
    }    
}

/***************************************************************************//**
* @brief Set display intensity (0...15) or special case 255 (0xFF)
*        (special case 255 is the minimum intensity plus all 8 digits active)
* @param intensity [in] display intensity (0..15)
*******************************************************************************/
void dispmax_intensity(int intensity)
{
    if(intensity < 0)
        intensity = 0;
    else if((intensity > 15) && (intensity != 255))
        intensity = 15;

    intensity_h = (uint8_t) intensity;

    prepare_control_tx();
}

/***************************************************************************//**
* @brief Display polling function. Must be called every program cycle
*******************************************************************************/
void dispmax_poll(const ustime_t sys_ustime)
{
    // Data to send?
    int tx_len = tx_cnt - tx_idx;
    if((tx_len > 0) && !spi_drv_is_busy())
    {
        if(tx_len > 2)
            tx_len = 2;
        if((tx_idx + tx_len) <= sizeof(tx_data))
        {
            if(spi_drv_send(&tx_data[tx_idx], tx_len))
                tx_idx += tx_len;
        }
        else{
            tx_cnt = tx_idx = 0;
        }
    }

    // Refresh display?
    if(get_diff_ustime(sys_ustime, sys_ustime_old) >= DISPMAX_REFRESH_TIME)
    {
        sys_ustime_old = sys_ustime;

        // Are there 2 pages to display? 
        if(frame_buffer[3] != 0)
        {
            // Display every page DISPMAX_PAGE_TIME [us] long
            if(get_diff_ustime(sys_ustime, page_sw_ustime) > DISPMAX_PAGE_TIME)
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

        prepare_digit_tx();
    }
}
