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
 * main - implements the main function and the basic algorithm/logic.
 ******************************************************************************/

//******************************************************************************
// Includes
//******************************************************************************
#include <string.h> // memset

#include "pico/stdlib.h"
#include "gpio_drv.h"
#include "in_out.h"
#include "ustime.h"
#include "spi_drv.h"
#include "i2c_drv.h"
#include "i2c_mem.h"
#include "i2c_rtc.h"
#include "i2c_bh1750.h"
#include "i2c_manager.h"
#include "dcf77.h"
#include "utils.h"
#include "test_btn.h"
#include "cli.h"
#include "cli_func.h"
#include "test_mem.h"
#include "hardware/watchdog.h"
#include "rtc_intern.h"

#include DISP_INCLUDE

//******************************************************************************
// Defines
//******************************************************************************

#define BOLD_RED_TEXT   "\033[1;31m"
#define NORMAL_TEXT     "\033[0m"

// Timeout (in seconds) for DCF in-sync flag
#define DCF_IN_SYNC_TOUT_S  43200UL     // 12 hours = 720 min = 43200 sec

#ifdef MAIN_DEBUG
#define MAIN_LOG(...)     DEBUG_PRINTF(__VA_ARGS__)
#define MAIN_LOG_TIME(prefix,dt,suffix) DATETIME_PRINTF_TIME(DEBUG_PRINTF,prefix,dt,suffix)
#define MAIN_LOG_DATE(prefix,dt,suffix) DATETIME_PRINTF_DATE(DEBUG_PRINTF,prefix,dt,suffix)
#define MAIN_LOG_DT(prefix,dt,suffix)   do { MAIN_LOG_TIME(prefix,dt,"  "); \
                                             MAIN_LOG_DATE("",dt,suffix); \
                                        } while(0)
#else
#define MAIN_LOG(...)    
#define MAIN_LOG_TIME(prefix,dt,suffix)
#define MAIN_LOG_DATE(prefix,dt,suffix)
#define MAIN_LOG_DT(prefix,dt,suffix)
#endif

// Date/Time source
typedef enum {
    dt_src_none = 0,
    dt_src_dcf,
    dt_src_rtc,
    dt_src_int
} dt_src_t;

typedef struct {
    datetime_t dt;      // Date/Time 
    bool received;      // Flag indicates new Date/Time received in this cycle
    bool in_sync;       // Flag indicates the Date/Time is in sync
    ustime_t ustime;    // System time (useconds) when Date/Time was received 
    s_time_t s_time;    // System time (seconds) when Date/Time was received 
    dt_src_t sync_src;  // Synchronization source (which date/time was used to sync)
} dt_t;

//******************************************************************************
// System time variables
ustime_t sys_ustime = 0UL;      // System time in us (since the board was powered-on)
s_time_t sys_s_time = 0UL;      // System time in seconds (since the board was powered-on)
ustime_t us_s_time_diff = 0UL;  // Variable used to detect 1 second change (since the board was powered-on)

// Display variables
ustime_t display_ustime = 0UL;  // Display refresh
int display_intensity = 8;      // Display intensity

// Convert lx value to display intensity
//                      0  1  2   3   4   5   6   7    8    9    10   11   12   13   14   15
const int lx_table[] = {0, 5, 10, 25, 40, 60, 80, 110, 140, 180, 220, 270, 320, 380, 440, 520};
const int lx_table_cnt = sizeof(lx_table) / sizeof(int);
int lx_to_display_intensity(int lx_value);

// Current Time/Date variables
dt_t dcf_dt;
dt_t rtc_dt;
dt_t int_dt;
dt_t fin_dt;

/***************************************************************************//**
* @brief Get System Time in us
*        Warning! This is not safe in case of multi-core
* @return System Time in us
*******************************************************************************/
uint32_t get_sys_ustime(void)
{
    uint32_t lr = timer_hw->timelr;
    timer_hw->timehr;
    return lr;
}

/***************************************************************************//**
* @brief Display function
*******************************************************************************/
void display(void)
{
    //TP_TGL(LOG_CH2);
    DISP_CLEAR();

    // Display Time (mode=0)
    if(cli_display == 0)
    {
        // Blink finaldate time if not in sync
        if(fin_dt.in_sync || (sys_ustime & 0x00080000))
        {
            DISP_TIME(&fin_dt.dt);
        }

        // Dot 0 : DCF state
        //         ON - not in sync
        //         BLINK - synchronizing (signal quality > 80%)
        //         OFF - in sync
        if(!dcf_dt.in_sync)
        {
            if(dcf_get_quality() > 80)
                DISP_DOT(0, (sys_ustime & 0x00080000));
            else 
                DISP_DOT(0, 1);
        }

        // Dot 1 : RTC state
        //         ON - RTC in sync
        //         OFF - RTC not in sync
        DISP_DOT(1, (rtc_dt.in_sync ? 0 : 1));

        // Dot 2 : Toggle every 0.524s
        DISP_DOT(2, (sys_ustime & 0x00080000));
    }
    // Other display modes
    else {
        switch(cli_display)
        {
            case 1: DISP_CLEAR(); break;
            case 2: DISP_HEX(0x1234, 6); break;
            case 3: DISP_PUTS("HELLO"); break;
            case 4: DISP_PUTS("TST"); break;
            case 5: DISP_PUTCH(0, 'G'); break;
            case 6: DISP_PUTCH(1, 'H'); break;
            case 7: DISP_PUTCH(3, 'A'); break;
            case 8: DISP_PUTCH(4, 'B'); break;
            case 9: DISP_PUTCH(7, 'C'); break;
            case 10: DISP_INT(display_intensity); break;
            case 11: DISP_INT(i2c_bh1750_get_val()); break;
            case 12: DISP_INT(dcf_get_quality()); break;
            default: DISP_INT(cli_display); break;
        }
    }
}

/***************************************************************************//**
* @brief Callback when I2C RTC Set finished
* @param result [in] RTC Set result (i2c_err_t converted to int)
*******************************************************************************/
void callback_i2c_rtc_set(int result)
{
    i2c_err_t i2c_err = (i2c_err_t) result;
    if(i2c_err == i2c_success)
    {
        rtc_dt.in_sync = true;
    }
}

/***************************************************************************//**
* @brief Clear the dt variable
* @param dt_ptr [in/out] pointer to dt variable to clear
*******************************************************************************/
void dt_clear(dt_t * dt_ptr)
{
    datetime_clear(&dt_ptr->dt);
    dt_ptr->received = false;
    dt_ptr->in_sync = false;
    dt_ptr->ustime = 0UL;
    dt_ptr->s_time = 0UL;
    dt_ptr->sync_src = dt_src_none;
}

/***************************************************************************//**
* @brief Set new received Date/Time to a dt variable
* @param datetime_ptr [in] pointer to received Date/Time data
* @param dt_ptr [out] pointer to dt variable to set
*******************************************************************************/
void dt_set_received(dt_t * dt_ptr, const datetime_t * datetime_ptr, dt_src_t sync_src)
{
    if(datetime_ptr)
    {
        datetime_copy(&dt_ptr->dt, datetime_ptr);
        dt_ptr->ustime = sys_ustime;
        dt_ptr->s_time = sys_s_time;
        dt_ptr->received = true;
        dt_ptr->sync_src = sync_src;
    }
}

/***************************************************************************//**
* @brief Check if the difference in seconds between two datetime variables 
*        is more than sec parameter
* @param dt1_ptr [in] pointer to datetime variable 1
* @param dt2_ptr [in] pointer to datetime variable 2
* @return true if the difference in seconds between two datetime variables 
*         is more than sec parameter or false if not
*******************************************************************************/
bool dt_diff_flag(const dt_t * dt1_ptr, const dt_t * dt2_ptr, int sec)
{
    int sec_diff = datetime_time_diff(&dt1_ptr->dt, &dt2_ptr->dt);
    return ((sec_diff > sec) || (sec_diff < -sec) 
        || (datetime_date_compare(&dt1_ptr->dt, &dt2_ptr->dt) != 0));
}

/***************************************************************************//**
* @brief Decide the final Date/Time
*
*   DCF has the top priority. If DCF received, use it as final date/time 
*   and check if RTC must be synchronized.
*
*   RTC has the second priority but is our main source in case there is no DCF.
*   Use RTC to set final date/time when there is no DCF.
*
*   Internal RTC is the last chance. Re-synchronize it if there is a valid 
*   date/time from DCF or RTC.
*******************************************************************************/
void dt_poll(void)
{
    bool fin_set_flag = false;

    // DCF
    if(dcf_dt.received)
    {
        MAIN_LOG_DT("Main: DCF: ",(dcf_dt.dt), "\r\n");

        // Use DCF as Final time
        dt_set_received(&fin_dt, &dcf_dt.dt, dt_src_dcf);
        fin_set_flag = true;

        // Check if RTC must be set (if more than 1 sec difference with DCF)
        if(dt_diff_flag(&dcf_dt, &rtc_dt, 1))
        {
            MAIN_LOG("Main: set RTC (DCF diff: %is)\r\n", datetime_time_diff(&dcf_dt.dt, &rtc_dt.dt));
            i2c_man_req_rtc_set(&dcf_dt.dt, callback_i2c_rtc_set);
            rtc_dt.in_sync = false;
        }

        dcf_dt.in_sync = true;
        dcf_dt.received = false;
    }

    // RTC
    if(rtc_dt.received)
    {
        if(!rtc_dt.in_sync)
        {
            // If RTC received but not in sync and DCF also not in sync
            // then RTC is our only time source now, mark it as "in sync"
            if(!dcf_dt.in_sync)
                rtc_dt.in_sync = true;
        }
        else if(!fin_set_flag)
        {
            // If RTC in sync, use it as Final time
            dt_set_received(&fin_dt, &rtc_dt.dt, dt_src_rtc);
            fin_set_flag = true;
        }

        rtc_dt.received = false;
    }

    // RTC-intern
    if(int_dt.received)
    {
        // If we have a valid Final time (not from internal RTC) use it to check 
        // if the internal RTC must be set (in case > 1 sec difference)
        if((fin_dt.in_sync || fin_set_flag) && (fin_dt.sync_src != dt_src_int))
        {
            if(dt_diff_flag(&int_dt, &fin_dt, 1))
            {
                MAIN_LOG_DT("Main: set RTC-intern: ", (fin_dt.dt), "\r\n");
                int_dt.in_sync = rtc_int_set(&fin_dt.dt);
            }
        }

        int_dt.received = false;
    }

    if(fin_set_flag)
        fin_dt.in_sync = true;
}

/***************************************************************************//**
* @brief Date/Time timeout (called every second)
*   Check if a date/time was not updated a period of time and set it as "not in sync".
*   If the final date/time was not updated more than 1 second from DCF or RTC,
*   use the internal RTC to update the final date/time.
*******************************************************************************/
void dt_s_tout(void)
{
    // Mark DCF not in sync if not received more than DCF_IN_SYNC_TOUT_S seconds
    if(dcf_dt.in_sync)
    {
        if(get_diff_s_time(sys_s_time, dcf_dt.s_time) > DCF_IN_SYNC_TOUT_S)
        {
            MAIN_LOG("Main: DCF not in sync\r\n");
            dcf_dt.in_sync = false;
        }
    }

    // Mark RTC not in sync if not received > 1 sec
    if(rtc_dt.in_sync)
    {
        if(get_diff_s_time(sys_s_time, rtc_dt.s_time) > 1)
        {
            MAIN_LOG("Main: RTC not in sync\r\n");
            rtc_dt.in_sync = false;
        }
    }

    // Mark RTC-intern not in sync if not received > 1 sec
    if(int_dt.in_sync)
    {
        if(get_diff_s_time(sys_s_time, int_dt.s_time) > 1)
        {
            MAIN_LOG("Main: RTC-intern not in sync\r\n");
            int_dt.in_sync = false;
        }
    }

    // Check if final time is in sync
    // if not, use the internal RTC as final datetime
    if(fin_dt.in_sync)
    {
        if(get_diff_s_time(sys_s_time, fin_dt.s_time) > 1)
        {
            MAIN_LOG("Main: Final Time not in sync\r\n");
            fin_dt.in_sync = false;
        }
    }
    if(!fin_dt.in_sync)
    {
        // If final datetime not in sync, use the internal RTC (our last option)
        // consider final datetime in-sync only when the internal RTC is in sync
        dt_set_received(&fin_dt, &int_dt.dt, dt_src_int);
        fin_dt.in_sync = int_dt.in_sync;
    }
}

/***************************************************************************//**
* @brief Main function
*******************************************************************************/
int main()
{
    gpio_drv_init();
    io_init();
    spi_drv_init();
    DISP_INIT();
    i2c_drv_init();
    i2c_drv_set_utime_func(get_sys_ustime);

    test_mem_init();

    cli_init();
    cli_func_init();

    io_puts("Hello world, " BOLD_RED_TEXT "how are you" NORMAL_TEXT " today!\r\n");

    i2c_rtc_init();
    i2c_bh1750_init();
    i2c_man_init();
    dcf_init();
    
    rtc_int_init();

    if(watchdog_caused_reboot())
        io_puts(BOLD_RED_TEXT "Rebooted by watchdog" NORMAL_TEXT "\r\n");
    watchdog_enable(100, 1);

    dt_clear(&dcf_dt);
    dt_clear(&rtc_dt);
    dt_clear(&int_dt);
    dt_clear(&fin_dt);

    while (1)
    {
        sys_ustime = get_sys_ustime();

        // Detect 1 second change (1.000.000 us)
        bool second_changed = false;
        if(get_diff_ustime(sys_ustime, us_s_time_diff) >= 1000000)
        {   
            us_s_time_diff = sys_ustime;
            sys_s_time++;
            second_changed = true;
        }

        TP_TGL(LOG_CH2);

        cli_poll();

        // I2C RTC, BH1750, memory poll
        i2c_man_update_t updated_val = i2c_man_poll(sys_ustime);
        if(updated_val == i2c_man_update_rtc)
        {
            dt_set_received(&rtc_dt, i2c_rtc_get_datetime(), dt_src_rtc);
        }

        // DCF poll
        if(dcf_poll(sys_ustime))
        {
            dt_set_received(&dcf_dt, dcf_get_datetime(), dt_src_dcf);
        }

        // RTC Intern poll
        if(rtc_int_poll(sys_ustime))
        {
            dt_set_received(&int_dt, rtc_int_get_datetime(), dt_src_int);
        }

        // Decide the final Date/Time
        dt_poll();
        if(second_changed)
        {
            dt_s_tout();
        }

        // Display intensity (only if intensity override is not active)
        if((updated_val == i2c_man_update_bh1750) && (cli_intens == -1))
        {
            int lx_new = i2c_bh1750_get_val();
            int disp_intens_new = lx_to_display_intensity(lx_new);
            if(disp_intens_new != display_intensity)
            {
                if(disp_intens_new > display_intensity)
                    display_intensity++;
                else
                    display_intensity--;
                DISP_INTENS(display_intensity);
            }
        }
        // Refresh display data
        else if(get_diff_ustime(sys_ustime, display_ustime) >= 50000)
        {   
            display_ustime = sys_ustime;
            display();
        }
        DISP_POLL(sys_ustime);

        if(cli_test_val1 == 0)
        {
            watchdog_update();
        }
    }
}

/***************************************************************************//**
* @brief Convert lx value to display intensity
* @param lx_value [in] lx value
* @return new display intensity based on lx
*******************************************************************************/
int lx_to_display_intensity(int lx_value)
{
    int idx;
    for(idx = 1; idx < lx_table_cnt; idx++)
    {
        if(lx_value < lx_table[idx])
            break;
    }
    return (idx - 1);
}

