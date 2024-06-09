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
 * i2c_rtc - the driver for the DS3231 RTC.
 * It uses i2c_drv module for the I2C communication.
 ******************************************************************************/

//******************************************************************************
// Includes
//******************************************************************************
#include <stdint.h>
#include <string.h> // memcpy
#include <stdio.h>

#include "pico/stdlib.h"

#include "i2c_rtc.h"
#include "utils.h"

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// Global Variables
//******************************************************************************

/*
*       Bit |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
*   --------+-------+-------+-------+-------+-------+-------+-------+-------+---------------
*    Byte 0 |   0   |    10 x Seconds       |           1 x Seconds         | Seconds 00..59
*    Byte 1 |   0   |    10 x Minutes       |           1 x Minutes         | Minutes 00..59
*    Byte 2 |   0   | 12/24 |   10 x Hours  |           1 x Hours           | Hours 00..23
*   --------+-------+-------+-------+-------+-------+-------+-------+-------+---------------
*    Byte 3 |   0   |   0   |   0   |   0   |   0   |      Day (1..7)       | Day 1..7
*    Byte 4 |   0   |   0   |   10 x Date   |           1 x Date            | Date 1..31
*    Byte 5 | Cent. |   0   |   0   | 10xM. |           1 x Month           | Month 1..12 + Century
*    Byte 6 |           10 x Years          |           1 x Years           | Year 00..99
*   --------+-------+-------+-------+-------+-------+-------+-------+-------+---------------
*    Byte 7 | A1M1  |    10 x Seconds       |           1 x Seconds         | Alarm1 Seconds 00..59
*    Byte 8 | A1M2  |    10 x Minutes       |           1 x Minutes         | Alarm1 Minutes 00..59
*    Byte 9 | A1M3  | 12/24 |   10 x Hours  |           1 x Hours           | Alarm1 Hours 00..23
*    Byte10 | A1M4  | DY/DT |   10 x Date   |           1 x Date/Day        | Alarm1 Date 1..31 / Day 1..7
*   --------+-------+-------+-------+-------+-------+-------+-------+-------+---------------
*    Byte11 | A2M2  |    10 x Minutes       |           1 x Minutes         | Alarm2 Minutes 00..59
*    Byte12 | A2M3  | 12/24 |   10 x Hours  |           1 x Hours           | Alarm2 Hours 00..23
*    Byte13 | A2M4  | DY/DT |   10 x Date   |           1 x Date/Day        | Alarm2 Date 1..31 / Day 1..7
*   --------+-------+-------+-------+-------+-------+-------+-------+-------+---------------
*    Byte14 | /EOSC | BBSQW |  CONV |  RS2  |  RS1  | INTCN |  A2IE |  A1IE | Control
*    Byte15 |  OSF  |   0   |   0   |   0   | EN32k |  BSY  |  A2F  |  A1F  | Control/Status
*   --------+-------+-------+-------+-------+-------+-------+-------+-------+---------------
*    Byte16 |  SIGN |                       Data                            | Aging Offset
*   --------+-------+-------+-------+-------+-------+-------+-------+-------+---------------
*    Byte17 |  SIGN |                       Data                            | MSB of Temperature
*    Byte18 |      Data     |   0   |   0   |   0   |   0   |   0   |   0   | LSB of Temperature
*/
static uint8_t rtc_rx_raw[17];      // buffer used to read RTC raw data (17 registers)
static uint8_t rtc_tx_raw[1 + 17];  // buffer used to write RTC raw data (1 addr + 17 registers)

static datetime_t act_datetime;     // last read datetime
static uint16_t act_ctrl_st = I2C_RTC_CTL_INVALID;    // last read control/status

/***************************************************************************//**
* @brief Init i2c RTC Driver. Must be called in main in init phase
*******************************************************************************/
void i2c_rtc_init(void)
{

}

/***************************************************************************//**
* @brief Extract datetime from memory (coded as bcd format)
*
*       Bit |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
*   --------+-------+-------+-------+-------+-------+-------+-------+-------+
*    Byte 0 |   0   |    10 x Seconds       |           1 x Seconds         |
*    Byte 1 |   0   |    10 x Minutes       |           1 x Minutes         |
*    Byte 2 |   0   | 12/24 |   10 x Hours  |           1 x Hours           |
*    Byte 3 |   0   |   0   |   0   |   0   |   0   |      Day (1..7)       |
*    Byte 4 |   0   |   0   |   10 x Date   |           1 x Date            |
*    Byte 5 | Cent. |   0   |   0   | 10xM. |           1 x Month           |
*    Byte 6 |           10 x Years          |           1 x Years           |
*
* @param ptr_time [out] pointer to datetime_t variable to copy the extracted data
* @param mem [in] pointer to memory from where to extract the datetime
* @param mem_size [in] maximal size of memory in bytes
* @return Count of extracted bytes or -1 in case of error
*******************************************************************************/
static int rtc_mem_to_datetime(datetime_t * ptr_datetime, const uint8_t * mem, int mem_size)
{
    datetime_t dt;

    if(mem_size < 7)
        return -1;

    dt.sec = bcd_to_int8(mem[0]);
    dt.min = bcd_to_int8(mem[1]);
    dt.hour = bcd_to_int8(mem[2]);
    dt.dotw = bcd_to_int8(mem[3]);

    // Convert RTC (Mon=1... Sun=7) to datetime_t (Sun=0, Mon=1..Sat=6)
    if(dt.dotw == 7)
        dt.dotw = 0;

    dt.day = bcd_to_int8(mem[4]);
    dt.month = bcd_to_int8(mem[5] & 0x7F);
    dt.year = (int16_t) bcd_to_int8(mem[6]);
    dt.year += 2000;

    if(!datetime_is_valid(&dt))
        return -1;

    datetime_copy(ptr_datetime, &dt);

    return 7;
}

/***************************************************************************//**
* @brief Extract alarm1 from memory (coded as bcd format) 
*
*       Bit |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
*   --------+-------+-------+-------+-------+-------+-------+-------+-------+
*    Byte 0 | A1M1  |    10 x Seconds       |           1 x Seconds         | Alarm1 Seconds 00..59
*    Byte 1 | A1M2  |    10 x Minutes       |           1 x Minutes         | Alarm1 Minutes 00..59
*    Byte 2 | A1M3  | 12/24 |   10 x Hours  |           1 x Hours           | Alarm1 Hours 00..23
*    Byte 3 | A1M4  | DY/DT |   10 x Date   |           1 x Date/Day        | Alarm1 Date 1..31 / Day 1..7
*
* @param mem [in] pointer to memory from where to extract the alarm
* @param mem_size [in] maximal size of memory in bytes
* @return Count of extracted bytes or -1 in case of error
*******************************************************************************/
static int rtc_mem_to_alarm1(const uint8_t * mem, int mem_size)
{
    if(mem_size < 4)
        return -1;

    //!!! TODO (not yet implemented)

    return 4;
}

/***************************************************************************//**
* @brief Extract alarm2 from memory (coded as bcd format) 
*
*       Bit |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
*   --------+-------+-------+-------+-------+-------+-------+-------+-------+
*    Byte 0 | A2M2  |    10 x Minutes       |           1 x Minutes         | Alarm2 Minutes 00..59
*    Byte 1 | A2M3  | 12/24 |   10 x Hours  |           1 x Hours           | Alarm2 Hours 00..23
*    Byte 2 | A2M4  | DY/DT |   10 x Date   |           1 x Date/Day        | Alarm2 Date 1..31 / Day 1..7
*
* @param mem [in] pointer to memory from where to extract the alarm
* @param mem_size [in] maximal size of memory in bytes
* @return Count of extracted bytes or -1 in case of error
*******************************************************************************/
static int rtc_mem_to_alarm2(const uint8_t * mem, int mem_size)
{
    if(mem_size < 3)
        return -1;

    //!!! TODO (not yet implemented)

    return 3;
}

/***************************************************************************//**
* @brief Extract control/statu from memory
*
*       Bit |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
*   --------+-------+-------+-------+-------+-------+-------+-------+-------+
*    Byte 0 | /EOSC | BBSQW |  CONV |  RS2  |  RS1  | INTCN |  A2IE |  A1IE | Control
*    Byte 1 |  OSF  |   0   |   0   |   0   | EN32k |  BSY  |  A2F  |  A1F  | Control/Status
*
* @param mem [in] pointer to memory from where to extract the alarm
* @param mem_size [in] maximal size of memory in bytes
* @return Count of extracted bytes or -1 in case of error
*******************************************************************************/
static int rtc_mem_to_ctrl(uint16_t * ptr_ctrl, const uint8_t * mem, int mem_size)
{
    if(mem_size < 2)
        return -1;

    uint16_t ctrl = (uint16_t) mem[0];
    ctrl <<= 8;
    ctrl |= (uint16_t) mem[1];

    *ptr_ctrl = ctrl;

    return 2;
}

/***************************************************************************//**
* @brief Copy datetime variable to memory (code it in bcd format)
* @param mem [out] pointer to memory where to copy the time
* @param mem_size [in] maximal size of memory in bytes
* @param ptr_datetime [in] pointer to datetime variable from where to copy data
* @return Count of copied bytes or -1 in case of error
*******************************************************************************/
static int rtc_datetime_to_mem(uint8_t * mem, int mem_size, const datetime_t * ptr_datetime)
{
    if((mem_size < 7) || (!datetime_is_valid(ptr_datetime)))
        return -1;

    mem[0] = int8_to_bcd(ptr_datetime->sec);
    mem[1] = int8_to_bcd(ptr_datetime->min);
    mem[2] = int8_to_bcd(ptr_datetime->hour);
    mem[3] = int8_to_bcd(ptr_datetime->dotw);

    // Convert datetime_t (Sun=0, Mon=1..Sat=6) to RTC (Mon=1... Sun=7)
    if(mem[3] == 0)
        mem[3] = 7;

    mem[4] = int8_to_bcd(ptr_datetime->day);
    mem[5] = int8_to_bcd(ptr_datetime->month);
    mem[6] = int8_to_bcd((int8_t)(ptr_datetime->year % 100));

    return 7;
}

/***************************************************************************//**
* @brief Extract all rtc data from memory (after all registers are read)
* @param mem [in] pointer to memory from where to extract the data
* @param mem_size [in] maximal size of memory in bytes
* @return true in case all data successfully extracted or false in case of error
*******************************************************************************/
static bool rtc_extract_from_mem(const uint8_t * mem, int mem_size)
{
    // Extract datetime
    datetime_t dt;
    int cnt = 0;
    int res = rtc_mem_to_datetime(&dt, &mem[0], mem_size);

    if(res < 0)
    {
        I2C_RTC_LOG("i2c_rtc_read_poll: bcd err datetime\r\n");
        return false;
    }

    // Extract Alarm1
    cnt += res;
    res = rtc_mem_to_alarm1(&mem[cnt], mem_size - cnt);
    if(res < 0)
    {
        I2C_RTC_LOG("i2c_rtc_read_poll: cannot read alarm1\r\n");
        return false;
    }

    // Extract Alarm2
    cnt += res;
    res = rtc_mem_to_alarm2(&mem[cnt], mem_size - cnt);
    if(res < 0)
    {
        I2C_RTC_LOG("i2c_rtc_read_poll: cannot read alarm2\r\n");
        return false;
    }

    // Extract Control/Status
    uint16_t ctrl;
    cnt += res;
    res = rtc_mem_to_ctrl(&ctrl, &mem[cnt], mem_size - cnt);
    if(res < 0)
    {
        I2C_RTC_LOG("i2c_rtc_read_poll: cannot read ctrl/status\r\n");
        return false;
    }

    //I2C_RTC_LOG("%2.2i:%2.2i:%2.2i  ", dt.hour, dt.min, dt.sec);
    //I2C_RTC_LOG("%2.2i.%2.2i.%2.2i  ", dt.day, dt.month, dt.year);
    //I2C_RTC_LOG("(%i)\r\n", dt.dotw);
    I2C_RTC_LOG_TIME("", dt, "  ");
    I2C_RTC_LOG_DATE("", dt, "\r\n");
    I2C_RTC_LOG("ctrl=%04x\r\n", ctrl);
    datetime_copy(&act_datetime, &dt);
    act_ctrl_st = ctrl;
    return true;
}

/***************************************************************************//**
* @brief Start reading RTC in non blocking mode (the execution of program is 
*        not blocked and the result of reading must be polled with i2c_rtc_read_poll)
* @return i2c_success - reading process has started check result with i2c_rtc_read_poll, 
*         or i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_rtc_read_start(void)
{
    // Set address and initiate read
    uint8_t reg_addr = 0;
    if(!i2c_drv_transfer_start(I2C_RTC_DEV_ADDR, &reg_addr, 1, sizeof(rtc_rx_raw)))
    {
        I2C_RTC_LOG("i2c_rtc_read: busy\r\n");
        return i2c_err_busy;
    }

    return i2c_success;
}

/***************************************************************************//**
* @brief Polling the status of read RTC in non blocking mode (which has been started
*        with i2c_rtc_read_start function)
* @return i2c_success - reading process has finished with success, 
*         i2c_err_busy - reading process is still busy, poll it again later,
*         i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_rtc_read_poll(void)
{
    switch(i2c_drv_poll_state())
    {
    case i2c_state_busy:
        return i2c_err_busy;

    case i2c_state_full:
        I2C_RTC_LOG("i2c_rtc_read_poll: i2c_state_full\r\n");
        if(i2c_drv_get_rx_data(rtc_rx_raw, sizeof(rtc_rx_raw)) == sizeof(rtc_rx_raw))
        {
            I2C_RTC_DUMP(rtc_rx_raw, sizeof(rtc_rx_raw), 0UL);
            if(!rtc_extract_from_mem(rtc_rx_raw, sizeof(rtc_rx_raw)))
                return i2c_err_format;
            return i2c_success;
        }
        // Error, received less data than requested
        I2C_RTC_LOG("i2c_rtc_read_poll: err_length\r\n");
        return i2c_err_length;

    case i2c_state_abort:
        I2C_RTC_LOG("i2c_rtc_read_poll: err_abort\r\n");
        return i2c_err_abort;

    case i2c_state_tout:
        I2C_RTC_LOG("i2c_rtc_read_poll: err_tout\r\n");
        return i2c_err_tout;
    
    default:
        break;
    }

    I2C_RTC_LOG("i2c_rtc_read_poll: i2c_err_unknown\r\n");
    return i2c_err_unknown;
}

/***************************************************************************//**
* @brief Read RTC in blocking mode (block program cycle until the transfer finishes)
* @return i2c_success - reading process has started check result with i2c_rtc_read_poll, 
*         or i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_rtc_read_blocking(void)
{
    i2c_err_t res = i2c_rtc_read_start();

    if(res != i2c_success)
        return res;

    do 
    {
        tight_loop_contents();
        res = i2c_rtc_read_poll();
    } 
    while(res == i2c_err_busy);

    return res;
}

/***************************************************************************//**
* @brief Start write RTC in non blocking mode (the execution of program is 
*        not blocked and the status of writing must be polled with i2c_rtc_write_poll)
* @param ptr_datetime [in] - pointer to datetime to write
* @return i2c_success - writing process has started check status with i2c_rtc_read_poll, 
*         or i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_rtc_write_start(const datetime_t * ptr_datetime)
{
    // Copy time and date to a memory buffer
    memset(rtc_tx_raw, 0, sizeof(rtc_tx_raw));  // idx 0 = register address = 0

    int cnt = rtc_datetime_to_mem(&rtc_tx_raw[1], sizeof(rtc_tx_raw) - 1, ptr_datetime);
    if(cnt < 0)
    {
        I2C_RTC_LOG("error: cannot convert time to memory\r\n");
        return i2c_err_format;
    }

    I2C_RTC_DUMP(&rtc_tx_raw[1], sizeof(rtc_tx_raw) - 1, 0UL);

    // Initiate write
    if(!i2c_drv_transfer_start(I2C_RTC_DEV_ADDR, rtc_tx_raw, sizeof(rtc_tx_raw), 0))
    {
        I2C_RTC_LOG("i2c_rtc_set: busy\r\n");
        return i2c_err_busy;
    }

    return i2c_success;
}

/***************************************************************************//**
* @brief Polling the status of write RTC in non blocking mode (which has been started
*        with i2c_rtc_write_start function)
* @return i2c_success - writing process has finished with success, 
*         i2c_err_busy - writing process is still busy, poll it again later,
*         i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_rtc_write_poll(void)
{
    switch(i2c_drv_poll_state())
    {
    case i2c_state_busy:
        return i2c_err_busy;

    case i2c_state_idle:
        I2C_RTC_LOG("i2c_rtc_write_poll: success\r\n");
        return i2c_success;

    case i2c_state_abort:
        I2C_RTC_LOG("i2c_rtc_write_poll: err_abort\r\n");
        return i2c_err_abort;

    case i2c_state_tout:
        I2C_RTC_LOG("i2c_rtc_write_poll:err_tout\r\n");
        return i2c_err_tout;
    
    default:
        break;
    }

    I2C_RTC_LOG("i2c_rtc_write_poll: i2c_err_unknown\r\n");
    return i2c_err_unknown;
}

/***************************************************************************//**
* @brief Write RTC in blocking mode (block program cycle until the transfer finishes)
* @param ptr_time [in] - pointer to RTC time to write
* @param ptr_date [in] - pointer to RTC date to write
* @return i2c_success - writing process has finished with success, 
*         or i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_rtc_write_blocking(const datetime_t * ptr_datetime)
{
    i2c_err_t res = i2c_rtc_write_start(ptr_datetime);

    if(res != i2c_success)
        return res;

    do 
    {
        tight_loop_contents();
        res = i2c_rtc_write_poll();
    } 
    while(res == i2c_err_busy);

    return res;
}

/***************************************************************************//**
* @brief Return actual datetime
* @return pointer to actual datetime
*******************************************************************************/
datetime_t * i2c_rtc_get_datetime(void)
{
    return &act_datetime;
}
