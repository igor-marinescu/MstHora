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
#ifndef I2C_MAN_H
#define I2C_MAN_H

//******************************************************************************
// Includes
//******************************************************************************
#include "ustime.h"
#include "datetime_utils.h"
#include "test_mem.h"

#ifdef I2C_MAN_DEBUG
#include DEBUG_INCLUDE
#endif

//******************************************************************************
// Defines
//******************************************************************************
#ifdef I2C_MAN_DEBUG
#define I2C_MAN_LOG(...)     DEBUG_PRINTF(__VA_ARGS__)
#else
#define I2C_MAN_LOG(...)    
#endif

// Timeout for re-init in case if failed to init BH1750 (in ms)
#define I2C_MAN_BH1750_INIT_TOUT    230

// Timeout for cyclically reading BH1750 value
#define I2C_MAN_BH1750_READ_TOUT    230

// Timeout to cyclically poll RTC (in ms)
#define I2C_MAN_RTC_POLL_TOUT       100

// Pointer to callback function 
typedef void (*i2c_man_callback_t)(int result);

// Updated value
typedef enum {
    i2c_man_update_none = 0,    // Nothing is updated
    i2c_man_update_rtc,         // RTC is updated
    i2c_man_update_bh1750,      // BH1750 is updated
} i2c_man_update_t;

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init i2c Manager Module. Must be called in main in init phase
void i2c_man_init(void);

// Poll i2c Manager Module. Must be called every program cycle
i2c_man_update_t i2c_man_poll(const ustime_t sys_ustime);

// Request to read RTC
bool i2c_man_req_rtc_read(i2c_man_callback_t callback);

// Request to set RTC
bool i2c_man_req_rtc_set(const datetime_t * datetime_ptr, i2c_man_callback_t callback);

// Request to test memory
bool i2c_man_req_mem_test(const test_mem_req_t * req_ptr, i2c_man_callback_t callback);

// Request to init BH1750 module
bool i2c_man_req_bh1750_init(i2c_man_callback_t callback);

// Request to read BH1750 value
bool i2c_man_req_bh1750_read(i2c_man_callback_t callback);

//******************************************************************************
#endif /* I2C_MAN_H */
