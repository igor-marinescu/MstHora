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
 * i2c_bh1750 - the driver for the BH1750FVI ambient light sensor.
 * It uses i2c_drv module for the I2C communication.
 ******************************************************************************/
#ifndef I2C_BH1750_H
#define I2C_BH1750_H

//******************************************************************************
// Includes
//******************************************************************************
#include "i2c_drv.h"

#ifdef I2C_BH1750_DEBUG
#include DEBUG_INCLUDE
#endif

//******************************************************************************
// Defines
//******************************************************************************
#define BH1750_DEV_ADDR    0x23

#ifdef I2C_BH1750_DEBUG
#define I2C_BH1750_LOG(...)                DEBUG_PRINTF(__VA_ARGS__)
#define I2C_BH1750_DUMP(buff, len, addr)   DEBUG_DUMP((buff), (len), (addr))
#else
#define I2C_BH1750_LOG(...)    
#define I2C_BH1750_DUMP(buff, len, addr)
#endif

// Control/Status register
#define BH1750_CTL_POWER_DOWN   0x00    // Power Down
#define BH1750_CTL_POWER_ON     0x01    // Power On, waiting for measurement cmd
#define BH1750_CTL_RESET        0x07    // Reset data register value
#define BH1750_CTL_CONT_H_MODE  0x10    // Continuously measurement at 1lx resolution
#define BH1750_CTL_CONT_H_MODE2 0x11    // Continuously measurement at 0.5lx resolution
#define BH1750_CTL_CONT_L_MODE  0x13    // Continuously measurement at 4lx resolution
#define BH1750_CTL_ONCE_H_MODE  0x20    // One time measurement at 1lx resolution
#define BH1750_CTL_ONCE_H_MODE2 0x21    // One time measurement at 0.5lx resolution
#define BH1750_CTL_ONCE_L_MODE  0x23    // One time measurement at 4lx resolution

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init BH1750 Driver. Must be called in main in init phase
void i2c_bh1750_init(void);

// Start write BH1750 command in non blocking mode
i2c_err_t i2c_bh1750_cmd_start(const uint8_t cmd);

// Polling the status of write BH1750 command in non blocking mode
i2c_err_t i2c_bh1750_cmd_poll(void);

// Start reading BH1750 in non blocking mode
i2c_err_t i2c_bh1750_read_start(void);

// Polling the status of read BH1750 in non blocking mode 
i2c_err_t i2c_bh1750_read_poll(void);

// Return last received light value
int i2c_bh1750_get_val(void);

//******************************************************************************
#endif /* I2C_BH1750_H */
