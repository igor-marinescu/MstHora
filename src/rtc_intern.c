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
 * rtc_intern - the driver for the internal (integrated) RTC module.
 ******************************************************************************/

//******************************************************************************
// Includes
//******************************************************************************
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "rtc_intern.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

#include "gpio_drv.h"   //!!! to be deleted
#include "pico/stdlib.h"//!!! to be deleted

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// Global Variables
//******************************************************************************

static datetime_t act_datetime; // last read datetime

static ustime_t refresh_ustime; // Timer used to refresh (update) time and date

static datetime_t int_datetime; // Read date/time

/***************************************************************************//**
* @brief Init RTC intern module
*******************************************************************************/
void rtc_int_init(void)
{
    rtc_init();

    datetime_clear(&act_datetime);

    if(!rtc_set_datetime(&act_datetime))
    {
        RTC_INT_LOG("rtc_int: cannot set datetime\r\n");
    }

    if(rtc_running())
    {
        RTC_INT_LOG("rtc_int: running\r\n");
    }
    else{
        RTC_INT_LOG("rtc_int: not running\r\n");
    }
}

/***************************************************************************//**
* @brief RTC intern module polling function. Must be called every program cycle
* @param sys_ustime [in] System time in us
*******************************************************************************/
bool rtc_int_poll(const ustime_t sys_ustime)
{
    if(get_diff_ustime(sys_ustime, refresh_ustime) >= 130000)
    {   
        refresh_ustime = sys_ustime;

        if(rtc_get_datetime(&int_datetime))
        {
            datetime_copy(&act_datetime, &int_datetime);
            TP_TGL(LOG_CH3);    //!!! to be deleted
            return true;
        }
    }
    return false;
}

/***************************************************************************//**
* @brief Set time and date to RTC intern module
* @param ptr_datetime [in] - pointer to RTC intern datetime to set
* @return true in case of success or false in case of failure
*******************************************************************************/
bool rtc_int_set(datetime_t * ptr_datetime)
{
    bool ok = rtc_set_datetime(ptr_datetime);
    if(ok)
    {
        datetime_copy(&act_datetime, ptr_datetime);
    }
    else {
        RTC_INT_LOG("rtc_int_set: cannot set datetime\r\n");
    }
    return ok;
}

/***************************************************************************//**
* @brief Return RTC intern actual time
* @return pointer to RTC intern actual time
*******************************************************************************/
datetime_t * rtc_int_get_datetime(void)
{
    return &act_datetime;
}
