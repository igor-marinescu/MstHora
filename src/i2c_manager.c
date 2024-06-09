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
 * i2c_manager - manages the I2C communication.
 * Decides which I2C module (RTC, BH1750, EEPROM) has access to the I2C driver
 * and can transfer data.
 ******************************************************************************/

//******************************************************************************
// Includes
//******************************************************************************
#include <stdint.h>
#include <string.h> // memcpy
#include <stdio.h>

#include "pico/stdlib.h"

#include "i2c_manager.h"
#include "i2c_rtc.h"
#include "i2c_bh1750.h"
#include "gpio_drv.h"   //!!! to be deleted

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// Typedefs
//******************************************************************************

// Commands
typedef enum {
    cmd_no = 0,         // No command
    cmd_rtc_read,       // Read RTC
    cmd_rtc_set,        // Set RTC
    cmd_bh1750_init,    // Init BH1750
    cmd_bh1750_read,    // Read BH1750 value
    cmd_mem_test        // Memory Test
} cmd_t;

// Request
typedef struct {
    cmd_t cmd;                      // Command to be executed
    i2c_man_callback_t callback;    // Callback when command finishes
    int idx;                        // Index, variable used to identify different steps
} req_t;

//******************************************************************************
// Global Variables
//******************************************************************************
static req_t req_new;       // New request to be executed
static req_t req_exe;       // Request being currently executed

static ustime_t ms1_ustime; // Detect 1ms change

static i2c_man_update_t updated_val = i2c_man_update_none;

// RTC read/set variables
static datetime_t rtc_dt;
static int rtc_poll_tout = 0;

// BH1750 sensor variables
static bool bh1750_init_flag = false;   // True if BH1750 successfuly initialised
static int bh1750_init_tout = 0;        // Timeout for re-init in case if failed to init BH1750
static int bh1750_read_tout = 0;        // Timeout for cyclically reading BH1750 value

/***************************************************************************//**
* @brief Init request
* @param req_ptr [out] pointer to request struct to initialize
* @param cmd [in] command to initialize the request with
* @param callback [in] callback to initialize the request with
*******************************************************************************/
static void init_req(req_t * req_ptr, const cmd_t cmd, const i2c_man_callback_t callback)
{
    req_ptr->cmd = cmd;
    req_ptr->callback = callback;
    req_ptr->idx = 0;
}

/***************************************************************************//**
* @brief Copy request
* @param req_ptr [out] pointer to destination request
* @param req_ptr [in] pointer to source request
*******************************************************************************/
static void copy_req(req_t * req_out, const req_t * req_in)
{
    memcpy(req_out, req_in, sizeof(req_t));
}

/***************************************************************************//**
* @brief Init i2c Manager Module. Must be called in main in init phase
*******************************************************************************/
void i2c_man_init(void)
{
    req_new.cmd = cmd_no;
    req_exe.cmd = cmd_no;
}

/***************************************************************************//**
* @brief Poll RTC read command
*******************************************************************************/
static void poll_cmd_rtc_read(void)
{
    i2c_err_t res = i2c_err_unknown;
    bool finish = false;

    if(req_exe.idx == 0)
    {
        // Start request
        req_exe.idx = 1;
        res = i2c_rtc_read_start();
        finish = (res != i2c_success);
    }
    else {
        // Poll request
        res = i2c_rtc_read_poll();
        finish = (res != i2c_err_busy);
    }

    if(finish)
    {
        if(res == i2c_success)
            updated_val = i2c_man_update_rtc;
        if(req_exe.callback != NULL)
            req_exe.callback((int) res);
        req_exe.cmd = cmd_no;
    }
}

/***************************************************************************//**
* @brief Poll RTC set command
*******************************************************************************/
static void poll_cmd_rtc_set(void)
{
    i2c_err_t res = i2c_err_unknown;
    bool finish = false;

    if(req_exe.idx == 0)
    {
        // Start request
        req_exe.idx = 1;
        res = i2c_rtc_write_start(&rtc_dt);
        finish = (res != i2c_success);
    }
    else {
        // Poll request
        res = i2c_rtc_write_poll();
        finish = (res != i2c_err_busy);
    }

    if(finish)
    {
        if(req_exe.callback != NULL)
            req_exe.callback((int) res);
        req_exe.cmd = cmd_no;
    }
}

/***************************************************************************//**
* @brief Poll BH1750 init command
*******************************************************************************/
static void poll_cmd_bh1750_init(void)
{
    i2c_err_t res = i2c_err_unknown;
    bool finish = false;

    switch(req_exe.idx)
    {
        case 0: // Start PowerOn
            req_exe.idx = 1;
            res = i2c_bh1750_cmd_start(BH1750_CTL_POWER_ON);
            finish = (res != i2c_success);
            break;

        case 1: // Poll PowerOn
            res = i2c_bh1750_cmd_poll();
            if(res == i2c_success)
                req_exe.idx = 2;
            else if(res != i2c_err_busy)
                finish = true;
            break;

        case 2: // Start Config Continuously measurement
            req_exe.idx = 3;
            res = i2c_bh1750_cmd_start(BH1750_CTL_CONT_H_MODE);
            finish = (res != i2c_success);
            break;

        case 3: // Poll Config Continuously measurement
            res = i2c_bh1750_cmd_poll();
            finish = (res != i2c_err_busy);
            break;

        default:
            finish = true;
            break;
    }

    if(finish)
    {
        if(req_exe.callback != NULL)
            req_exe.callback((int) res);
        req_exe.cmd = cmd_no;
    }
}

/***************************************************************************//**
* @brief Poll BH1750 read command
*******************************************************************************/
static void poll_cmd_bh1750_read(void)
{
    i2c_err_t res = i2c_err_unknown;
    bool finish = false;

    if(req_exe.idx == 0)
    {
        // Start Read
        req_exe.idx = 1;
        res = i2c_bh1750_read_start();
        finish = (res != i2c_success);
    }
    else{
        // Poll Read
        res = i2c_bh1750_read_poll();
        finish = (res != i2c_err_busy);
    }

    if(finish)
    {
        if(res == i2c_success)
            updated_val = i2c_man_update_bh1750;
        if(req_exe.callback != NULL)
            req_exe.callback((int) res);
        req_exe.cmd = cmd_no;
    }
}

/***************************************************************************//**
* @brief Poll memory test command
*******************************************************************************/
static void poll_cmd_mem_test(void)
{
    if(!test_mem_poll())
    {
        // Memory test finished?
        if(req_exe.callback != NULL)
            req_exe.callback(test_mem_is_error() ? 0 : 1);
        req_exe.cmd = cmd_no;
    }
}

/***************************************************************************//**
* @brief BH1750 init callback. Function called when BH1750 init finishes
* @param result [in] return status of the BH1750 init function
*           i2c_err_t converted to int
*******************************************************************************/
void i2c_man_bh1750_init_callback(int result)
{
    bh1750_init_flag = (((i2c_err_t)result) == i2c_success);
}

/***************************************************************************//**
* @brief Poll i2c Manager Module. Must be called every program cycle
* @param sys_ustime [in] system time in us
* @return the type of data that has been updated
*******************************************************************************/
i2c_man_update_t i2c_man_poll(const ustime_t sys_ustime)
{
    updated_val = i2c_man_update_none;

    switch(req_exe.cmd)
    {
        case cmd_rtc_read:
            poll_cmd_rtc_read();
            break;

        case cmd_rtc_set:
            poll_cmd_rtc_set();
            break;

        case cmd_mem_test:
            poll_cmd_mem_test();
            break;

        case cmd_bh1750_init:
            poll_cmd_bh1750_init();
            break;

        case cmd_bh1750_read:
            poll_cmd_bh1750_read();
            break;

        case cmd_no:
            // Is there a request to execute?
            if(req_new.cmd != cmd_no)
            {
                copy_req(&req_exe, &req_new);
                init_req(&req_new, cmd_no, NULL);
                break;
            }

            // Init BH1750 (if not yet initialised)
            if(!bh1750_init_flag && (bh1750_init_tout == 0))
            {
                i2c_man_req_bh1750_init(i2c_man_bh1750_init_callback);
                bh1750_init_tout = I2C_MAN_BH1750_INIT_TOUT;
                break;
            }

            // Read RTC
            if(rtc_poll_tout == 0)
            {
                TP_TGL(LOG_CH4); //!!! to be deleted
                i2c_man_req_rtc_read(NULL);
                rtc_poll_tout = I2C_MAN_RTC_POLL_TOUT;
                break;
            }

            // Read BH1750 value
            if(bh1750_read_tout == 0)
            {
                TP_TGL(LOG_CH5); //!!! to be deleted
                i2c_man_req_bh1750_read(NULL);
                bh1750_read_tout = I2C_MAN_BH1750_READ_TOUT;
                break;
            }
            break;

        default:
            req_exe.cmd = cmd_no;
            break;
    }

    // Detect 1ms (1000us) and manage timeouts
    if(get_diff_ustime(sys_ustime, ms1_ustime) > 1000UL)
    {
        ms1_ustime = sys_ustime;
        if(bh1750_init_tout > 0)
            bh1750_init_tout--;
        if(bh1750_read_tout > 0)
            bh1750_read_tout--;
        if(rtc_poll_tout > 0)
            rtc_poll_tout--;
    }

    return updated_val;
}

/***************************************************************************//**
* @brief Request to read RTC
* @param callback [in] function to be called when request finishes
* @return true
*******************************************************************************/
bool i2c_man_req_rtc_read(i2c_man_callback_t callback)
{
    init_req(&req_new, cmd_rtc_read, callback);
    return true;
}

/***************************************************************************//**
* @brief Request to set RTC
* @param callback [in] function to be called when request finishes
* @return true
*******************************************************************************/
bool i2c_man_req_rtc_set(const datetime_t * datetime_ptr, i2c_man_callback_t callback)
{
    datetime_copy(&rtc_dt, datetime_ptr);
    init_req(&req_new, cmd_rtc_set, callback);
    return true;
}

/***************************************************************************//**
* @brief Memory test request
* @param req_ptr [in] pointer to request structure
* @param callback [in] function to be called when request finishes
* @return true
*******************************************************************************/
bool i2c_man_req_mem_test(const test_mem_req_t * req_ptr, i2c_man_callback_t callback)
{
    test_mem_req(req_ptr);
    init_req(&req_new, cmd_mem_test, callback);
    return true;
}

/***************************************************************************//**
* @brief Request to init BH1750 module
* @param callback [in] function to be called when request finishes
* @return true
*******************************************************************************/
bool i2c_man_req_bh1750_init(i2c_man_callback_t callback)
{
    init_req(&req_new, cmd_bh1750_init, callback);
    return true;
}

/***************************************************************************//**
* @brief Request to read BH1750 value
* @param callback [in] function to be called when request finishes
* @return true
*******************************************************************************/
bool i2c_man_req_bh1750_read(i2c_man_callback_t callback)
{
    init_req(&req_new, cmd_bh1750_read, callback);
    return true;
}
