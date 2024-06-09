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
 * i2c_drv - the driver for I2C communication (in not blocking mode, 
 * not multi-thread safe)
 ******************************************************************************/
//******************************************************************************
//      Example use cases:
//
//      - Write a data buffer (wr_buff)
//
//          1. Initiate transfer:
//              i2c_drv_transfer_start(sl_addr, wr_buff, sizeof(wr_buff), 0);
//
//          2. Every program cycle check if the transfer finished:
//              i2c_state_t state = i2c_drv_poll_state();
//              if(state != i2c_state_busy)
//              { ...
//
//      - Write a data buffer (wr_buff) and after read a data buffer (rd_buff):
//
//          1. Initiate transfer:
//              i2c_drv_transfer_start(sl_addr, wr_buff, sizeof(wr_buff), sizeof(rd_buff));
//
//          2. Every program cycle check if the transfer finished (and the receive buffer is full):
//              i2c_state_t state = i2c_drv_poll_state();
//              if(state == i2c_state_full)
//              { ...
//
//          3. Copy the received data:
//                  int rx_len = i2c_drv_get_rx_data(rd_buff, sizeof(rd_buff));
//
//      - Read a data buffer (rd_buff):
//
//          1. Initiate transfer:
//              i2c_drv_transfer_start(sl_addr, NULL, 0, sizeof(rd_buff));
//
//          2. Every program cycle check if the transfer finished (and the receive buffer is full):
//              i2c_state_t state = i2c_drv_poll_state();
//              if(state == i2c_state_full)
//              { ...
//
//          3. Copy the received data:
//                  int rx_len = i2c_drv_get_rx_data(rd_buff, sizeof(rd_buff));
//
//******************************************************************************
#ifndef I2C_DRV_H
#define I2C_DRV_H

//******************************************************************************
// Includes
//******************************************************************************
#include "ustime.h"

#ifdef I2C_DRV_DEBUG
#include DEBUG_INCLUDE
#endif

//******************************************************************************
// Defines
//******************************************************************************
#define I2C_DRV_ID          i2c1
#define I2C_DRV_IRQ         I2C1_IRQ
#define I2C_DRV_IRQ_PRIO    PICO_HIGHEST_IRQ_PRIORITY
//#define I2C_DRV_BAUDRATE    100000ul
#define I2C_DRV_BAUDRATE    400000ul

// Time required to send a byte (1 start + 8 bit + 1 ack) (in usec)
#define I2C_DRV_UTIME_BYTE  ((1000000ul*10ul)/I2C_DRV_BAUDRATE)

// Time until the driver starts to send data. Practically measured (with analyzer) 
// between the call to i2c_drv_transfer_start() and the generated i2c start condition. 
// Tested with baudrate 100KHz and 400KHz, the time is ~ 75us.
#define I2C_DRV_UTIME_START 100ul

#define I2C_DRV_SDA_PIN     14
#define I2C_DRV_SCL_PIN     15

#define I2C_DRV_BUFF_LEN    256

#ifdef I2C_DRV_DEBUG
#define I2C_DRV_LOG(...)    DEBUG_PRINTF(__VA_ARGS__)
#else
#define I2C_DRV_LOG(...)
#endif

typedef enum {
    i2c_state_idle = 0,
    i2c_state_busy,
    i2c_state_full,
    i2c_state_abort,
    i2c_state_tout
} i2c_state_t;

typedef enum {
    i2c_success = 0,        // No error, Operation finished with success
    i2c_err_busy,           // Driver is busy
    i2c_err_abort,          // Driver hat aborted the communication
    i2c_err_tout,           // i2c communication has timed out
    i2c_err_length,         // Incorect length of received data
    i2c_err_unknown,        // Unknown error, incorrect status
    i2c_err_argument,       // Incorrect argumnet value in function call
    i2c_err_format,         // Incorrect data format
} i2c_err_t;

// Function type: get Time in us, used for timeout control
typedef ustime_t (*i2c_drv_utime_func_t)(void);

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init the i2c Driver. Must be called in main in init phase
void i2c_drv_init(void);

// Set i2c time function
void i2c_drv_set_utime_func(i2c_drv_utime_func_t func);

// Start i2c Transfer
bool i2c_drv_transfer_start(const uint8_t sl_addr, const uint8_t * tx_ptr, int tx_cnt, int rx_cnt);

// Polls and returns the state of the i2c interface
i2c_state_t i2c_drv_poll_state(void);

// Returns the last read i2c data (if there is any received)
int i2c_drv_get_rx_data(uint8_t * ptr_dst, int dst_max);

// Returns the abort source (as mask) of the last transfer
uint32_t i2c_drv_get_abort_source(void);

//******************************************************************************
#endif /* I2C_DRV_H */
