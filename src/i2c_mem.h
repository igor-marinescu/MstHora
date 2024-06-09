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
 * i2c_mem - the driver for the AT24C32/AT24C64 EEPROM.
 * It uses i2c_drv module for the I2C communication.
 ******************************************************************************/
#ifndef I2C_MEM_H
#define I2C_MEM_H

//******************************************************************************
// Includes
//******************************************************************************
#include "i2c_drv.h"

#ifdef I2C_MEM_DEBUG
#include DEBUG_INCLUDE
#endif

//******************************************************************************
// Defines
//******************************************************************************
#define I2C_MEM_DEV_ADDR    0x57
#define I2C_MEM_PAGE_SIZE   32

// In case the I2C memory is busy with writing, the new request will be aborted
// (answered with NAK) while attempting to access the device. 
// AT24C32 Datasheet says it takes max ~10ms to write a page.
// We retry to access the memory continuously, until it answers with ACK.
// The below defines the number of tries (in bytes) in case of a NAK:
//
//            nr_tries = 10ms / time_to_send_a_byte
//
#define I2C_MEM_ABORT_TRIES  10000ul/I2C_DRV_UTIME_BYTE

#ifdef I2C_MEM_DEBUG
#define I2C_MEM_LOG(...)     DEBUG_PRINTF(__VA_ARGS__)
#else
#define I2C_MEM_LOG(...)    
#endif

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init i2c Memory Driver. Must be called in main in init phase
void i2c_mem_init(void);

// Start read memory in non blocking mode
i2c_err_t i2c_mem_read_start(uint8_t * dst_ptr, const uint16_t src_addr, int len);

// Polling the status of read memory in non blocking mode
i2c_err_t i2c_mem_read_poll(void);

// Read memory in blocking mode (block program cycle until the transfer finishes)
i2c_err_t i2c_mem_read_blocking(uint8_t * dst_ptr, const uint16_t src_addr, int len);

// Start write memory in non blocking mode
i2c_err_t i2c_mem_write_start(const uint16_t dst_addr, const uint8_t * src_ptr, int len);

// Polling the status of write memory in non blocking mode
i2c_err_t i2c_mem_write_poll(void);

// Write memory in blocking mode (block program cycle until the transfer finishes)
i2c_err_t i2c_mem_write_blocking(const uint16_t dst_addr, const uint8_t * src_ptr, int len);

//******************************************************************************
#endif /* I2C_MEM_H */
