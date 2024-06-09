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
#ifndef I2C_RTC_H
#define I2C_RTC_H

//******************************************************************************
// Includes
//******************************************************************************
#include "datetime_utils.h"
#include "i2c_drv.h"

#ifdef I2C_RTC_DEBUG
#include DEBUG_INCLUDE
#endif

//******************************************************************************
// Defines
//******************************************************************************
#define I2C_RTC_DEV_ADDR    0x68

#ifdef I2C_RTC_DEBUG
#define I2C_RTC_LOG(...)                DEBUG_PRINTF(__VA_ARGS__)
#define I2C_RTC_DUMP(buff, len, addr)   DEBUG_DUMP((buff), (len), (addr))
#define I2C_RTC_LOG_TIME(prefix,dt,suffix)  DATETIME_PRINTF_TIME(DEBUG_PRINTF,prefix,dt,suffix)
#define I2C_RTC_LOG_DATE(prefix,dt,suffix)  DATETIME_PRINTF_DATE(DEBUG_PRINTF,prefix,dt,suffix)
#else
#define I2C_RTC_LOG(...)    
#define I2C_RTC_DUMP(buff, len, addr)
#define I2C_RTC_LOG_TIME(preifx,dt,suffix)
#define I2C_RTC_LOG_DATE(prefix,dt,suffix)
#endif

// Control/Status register
#define I2C_RTC_CTL_INVALID 0xFFFF  // Control/Status register is not valid
#define I2C_RTC_CTL_EOSC    0x8000
#define I2C_RTC_CTL_BBSQW   0x4000
#define I2C_RTC_CTL_CONV    0x2000
#define I2C_RTC_CTL_RS2     0x1000
#define I2C_RTC_CTL_RS1     0x0800
#define I2C_RTC_CTL_INTCN   0x0400
#define I2C_RTC_CTL_A2IE    0x0200
#define I2C_RTC_CTL_A1IE    0x0100
#define I2C_RTC_CTL_OSF     0x0080
#define I2C_RTC_CTL_EN32KHZ 0x0008
#define I2C_RTC_CTL_BSY     0x0004
#define I2C_RTC_CTL_A2F     0x0002
#define I2C_RTC_CTL_A1F     0x0001

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init i2c RTC Driver. Must be called in main in init phase
void i2c_rtc_init(void);

// Start reading RTC in non blocking mode
i2c_err_t i2c_rtc_read_start(void);

// Polling the status of read RTC in non blocking mode
i2c_err_t i2c_rtc_read_poll(void);

// Read RTC in blocking mode
i2c_err_t i2c_rtc_read_blocking(void);

// Start write RTC in non blocking mode
i2c_err_t i2c_rtc_write_start(const datetime_t * ptr_datetime);

// Polling the status of write RTC in non blocking mode
i2c_err_t i2c_rtc_write_poll(void);

// Write RTC in blocking mode
i2c_err_t i2c_rtc_write_blocking(const datetime_t * ptr_datetime);

// Return actual datetime
datetime_t * i2c_rtc_get_datetime(void);


//******************************************************************************
#endif /* I2C_RTC_H */
