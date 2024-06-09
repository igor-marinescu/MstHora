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

//******************************************************************************
// Includes
//******************************************************************************
#include <stdint.h>
#include "pico/stdlib.h"

#include "i2c_bh1750.h"
#include "utils.h"

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// Global Variables
//******************************************************************************

static uint8_t bh1750_rx_raw[2];    // Buffer used to read BH1750 raw data
static uint16_t bh1750_rx_val = 0;  // Received Light Value

/***************************************************************************//**
* @brief Init BH1750 Driver. Must be called in main in init phase
*******************************************************************************/
void i2c_bh1750_init(void)
{

}

/***************************************************************************//**
* @brief Start write BH1750 command in non blocking mode (the execution of program is 
*        not blocked and the status of writing must be polled with i2c_bh1750_write_poll)
* @param cmd [in] - command to write to BH1750 (see BH1750_CTL_...)
* @return i2c_success - writing process has started check status with i2c_bh1750_write_poll, 
*         or i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_bh1750_cmd_start(const uint8_t cmd)
{
    // Initiate write command
    if(!i2c_drv_transfer_start(BH1750_DEV_ADDR, &cmd, sizeof(cmd), 0))
    {
        I2C_BH1750_LOG("i2c_bh1750_cmd_start: busy\r\n");
        return i2c_err_busy;
    }

    return i2c_success;
}

/***************************************************************************//**
* @brief Polling the status of write BH1750 command in non blocking mode 
*        (which has been started with i2c_bh1750_cmd_start function)
* @return i2c_success - writing process has finished with success, 
*         i2c_err_busy - writing process is still busy, poll it again later,
*         i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_bh1750_cmd_poll(void)
{
    switch(i2c_drv_poll_state())
    {
    case i2c_state_busy:
        return i2c_err_busy;

    case i2c_state_idle:
        I2C_BH1750_LOG("i2c_bh1750_cmd_poll: success\r\n");
        return i2c_success;

    case i2c_state_abort:
        I2C_BH1750_LOG("i2c_bh1750_cmd_poll: err_abort\r\n");
        return i2c_err_abort;

    case i2c_state_tout:
        I2C_BH1750_LOG("i2c_bh1750_cmd_poll:err_tout\r\n");
        return i2c_err_tout;
    
    default:
        break;
    }

    I2C_BH1750_LOG("i2c_bh1750_cmd_poll: i2c_err_unknown\r\n");
    return i2c_err_unknown;
}

/***************************************************************************//**
* @brief Start reading BH1750 in non blocking mode (the execution of program is 
*        not blocked and the result of reading must be polled with i2c_bh1750_read_poll)
* @return i2c_success - reading process has started check result with i2c_bh1750_read_poll, 
*         or i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_bh1750_read_start(void)
{
    // Set address and initiate read
    if(!i2c_drv_transfer_start(BH1750_DEV_ADDR, NULL, 0, sizeof(bh1750_rx_raw)))
    {
        I2C_BH1750_LOG("i2c_bh1750_read_start: busy\r\n");
        return i2c_err_busy;
    }

    return i2c_success;
}

/***************************************************************************//**
* @brief Polling the status of read BH1750 in non blocking mode 
*        (which has been started with i2c_bh1750_read_start function)
* @return i2c_success - reading process has finished with success, 
*         i2c_err_busy - reading process is still busy, poll it again later,
*         i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_bh1750_read_poll(void)
{
    switch(i2c_drv_poll_state())
    {
    case i2c_state_busy:
        return i2c_err_busy;

    case i2c_state_full:
        I2C_BH1750_LOG("i2c_bh1750_read_poll: i2c_state_full\r\n");
        if(i2c_drv_get_rx_data(bh1750_rx_raw, sizeof(bh1750_rx_raw)) == sizeof(bh1750_rx_raw))
        {
            I2C_BH1750_DUMP(bh1750_rx_raw, sizeof(bh1750_rx_raw), 0UL);
            bh1750_rx_val = (uint16_t) bh1750_rx_raw[0];
            bh1750_rx_val <<= 8;
            bh1750_rx_val |= (uint16_t) bh1750_rx_raw[1];
            return i2c_success;
        }
        // Error, received less data than requested
        I2C_BH1750_LOG("i2c_bh1750_read_poll: err_length\r\n");
        return i2c_err_length;

    case i2c_state_abort:
        I2C_BH1750_LOG("i2c_bh1750_read_poll: err_abort\r\n");
        return i2c_err_abort;

    case i2c_state_tout:
        I2C_BH1750_LOG("i2c_bh1750_read_poll: err_tout\r\n");
        return i2c_err_tout;
    
    default:
        break;
    }

    I2C_BH1750_LOG("i2c_bh1750_read_poll: i2c_err_unknown\r\n");
    return i2c_err_unknown;
}

/***************************************************************************//**
* @brief Return last received light value
* @return last received light value
*******************************************************************************/
int i2c_bh1750_get_val(void)
{
    return (int) bh1750_rx_val;
}
