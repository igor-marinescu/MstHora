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

//******************************************************************************
// Includes
//******************************************************************************
#include <string.h>
#include <stdio.h>
#include <stdarg.h>     //varg

#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"

#include "i2c_drv.h"

//******************************************************************************
// Function Prototypes
//******************************************************************************
void i2c_drv_irq();

//******************************************************************************
// Global Variables
//******************************************************************************
static volatile i2c_state_t state = i2c_state_idle;     //!< i2c Status
static volatile i2c_state_t state_int = i2c_state_idle; //!< i2c Internal status (used only after transmission finishes)
static volatile uint32_t tx_abort_src = 0ul;
static i2c_drv_utime_func_t utime_func = NULL;

static ustime_t utime_start = 0ul;  //!< Time (in usec) when the transfer started
static ustime_t utime_txall = 0ul;  //!< Time (in usec) required for transfer   

static uint8_t wr_buff[I2C_DRV_BUFF_LEN];   //!< Buffer for data to write
static uint8_t rd_buff[I2C_DRV_BUFF_LEN];   //!< Buffer for read data 

static volatile int wr_cnt = 0;     //!< Number of bytes to write
static volatile int rd_cnt = 0;     //!< Number of bytes to read
static volatile int tx_all = 0;     //!< Total number of bytes to send (write + read)
static volatile int tx_idx = 0;     //!< Index of currently byte to send (write and read)
static volatile int rd_idx = 0;     //!< Currently read index

/***************************************************************************//**
* @brief Init the i2c Driver. Must be called in main in init phase
*******************************************************************************/
void i2c_drv_init(void)
{
    // init pins & i2c interface
    gpio_init(I2C_DRV_SDA_PIN);
    gpio_set_function(I2C_DRV_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_DRV_SDA_PIN);
    gpio_init(I2C_DRV_SCL_PIN);
    gpio_set_function(I2C_DRV_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_DRV_SCL_PIN);
    i2c_init(I2C_DRV_ID, I2C_DRV_BAUDRATE);

    // Disable all interrupts
    i2c_hw_t *hw = i2c_get_hw(I2C_DRV_ID);
    hw->intr_mask = 0;

    // Set up and enable the interrupt handlers
    irq_set_exclusive_handler(I2C_DRV_IRQ, i2c_drv_irq);
    irq_set_priority(I2C_DRV_IRQ, I2C_DRV_IRQ_PRIO);
    irq_set_enabled(I2C_DRV_IRQ, true);
}

/***************************************************************************//**
* @brief Set i2c time function
* @param fun [in] function which returns actual time in useconds
*******************************************************************************/
void i2c_drv_set_utime_func(i2c_drv_utime_func_t func)
{
    utime_func = func;
}

/***************************************************************************//**
* @brief i2c Driver Interrupt
*******************************************************************************/
void i2c_drv_irq(void)
{
    i2c_hw_t *hw = i2c_get_hw(I2C_DRV_ID);
    uint32_t intr_stat = hw->intr_stat;
    io_rw_32 tx_flags = 0;
    io_rw_32 tx_data = 0;
    io_rw_32 rx_data = 0;

    // Abort detected?
    if(intr_stat & I2C_IC_INTR_STAT_R_TX_ABRT_BITS)
    {
        tx_abort_src = hw->tx_abrt_source;
        // Mask reserved & unused bits
        tx_abort_src &= 0x0001FFFFul;
        // Clear TX_ABORT bit and source
        hw->clr_tx_abrt;
        state_int = i2c_state_abort;
        return;
    }

    // Stop detected? End of transmission
    if(intr_stat & I2C_IC_INTR_STAT_R_STOP_DET_BITS)
    {
        hw->clr_stop_det;
        state = (state_int == i2c_state_busy) ? i2c_state_idle : state_int;
        hw->intr_mask = 0;
    }

    // Something received?
    if(intr_stat & I2C_IC_INTR_STAT_R_RX_FULL_BITS)
    {
        // Read all available data
        while(hw->rxflr > 0ul)
        {
            rx_data = hw->data_cmd;
            if(rd_idx < I2C_DRV_BUFF_LEN)
                rd_buff[rd_idx] = (uint8_t) rx_data;

            // All received? Disable all interrupts
            if(++rd_idx >= rd_cnt)
                state_int = i2c_state_full;
        }
    }

    // Something to send?
    if(intr_stat & I2C_IC_INTR_STAT_R_TX_EMPTY_BITS)
    {
        // Transfer Write-Bytes
        if(tx_idx < wr_cnt)
        {
            // First write-byte to send? Send start
            if(tx_idx == 0)
                tx_flags = I2C_IC_DATA_CMD_RESTART_BITS;

            // Last write-byte to send? If there are no bytes to receive
            // send stop and disable interrupts
            if((tx_idx >= (wr_cnt - 1)) && (wr_cnt == tx_all))
            {
                tx_flags = I2C_IC_DATA_CMD_STOP_BITS;
            }

            if(tx_idx < I2C_DRV_BUFF_LEN)
                tx_data = (io_rw_32) wr_buff[tx_idx];

            hw->data_cmd = tx_flags | tx_data;
            tx_idx++;
        }
        // Transfer Read-Bytes
        else if(tx_idx < tx_all)
        {
            // First read-byte to send? Send start
            if(tx_idx == wr_cnt)
                tx_flags = I2C_IC_DATA_CMD_RESTART_BITS;

            // Last read-byte to send? Send stop
            if(tx_idx >= (tx_all - 1))
                tx_flags = I2C_IC_DATA_CMD_STOP_BITS;

            hw->data_cmd = tx_flags | I2C_IC_DATA_CMD_CMD_BITS;
            tx_idx++;
        }
        else {
            // Clear tx empty interrupt
            hw->intr_mask = hw->intr_mask & ~I2C_IC_INTR_MASK_M_TX_EMPTY_BITS;
        }
    }
}

/***************************************************************************//**
* @brief Start i2c Transfer
* @param sl_addr [in] slave address
* @param wr_ptr [in] pointer to data buffer to write
* @param wr_len [in] length (in bytes) of data to write
* @param rd_len [in] length (in bytes) of data to read
* @return true - if the transfer has started  
*         false - error starting the transfer (i2c is busy)
*******************************************************************************/
bool i2c_drv_transfer_start(const uint8_t sl_addr, const uint8_t * wr_ptr, int wr_len, int rd_len)
{
    i2c_hw_t *hw = i2c_get_hw(I2C_DRV_ID);

    if(state == i2c_state_busy)
        return false;

    hw->enable = 0;
    hw->tar = (io_rw_32) sl_addr;
    hw->enable = 1;
    // Clear all interrupts
    hw->clr_intr;

    tx_abort_src = 0ul;
    state = state_int = i2c_state_idle;

    tx_idx = 0;
    rd_idx = 0;
    wr_cnt = 0;
    rd_cnt = 0;

    if(rd_len > 0)
        rd_cnt = rd_len;

    // Copy data to send
    if((wr_len > 0) && (wr_ptr != NULL))
    {
        if(wr_len > I2C_DRV_BUFF_LEN)
            wr_len = I2C_DRV_BUFF_LEN;

        memcpy(wr_buff, wr_ptr, wr_len);
        wr_cnt = wr_len;
    }

    // The total bytes to transfer = wr bytes (wr_cnt) + rd bytes (rd_cnt)
    tx_all = wr_cnt + rd_cnt;

    // Timeout control?
    if(utime_func != NULL)
    {
        utime_start = utime_func();
        // Time (in usec) required to finish the transfer (with double reserve)
        utime_txall = (ustime_t)(I2C_DRV_UTIME_START + (I2C_DRV_UTIME_BYTE * (ustime_t)tx_all * 2ul));
    }

    // Unmask necessary interrupts (this will julp in interrupt an will start sending data)
    if(tx_all > 0)
    {
        state = state_int = i2c_state_busy;
        state_int = i2c_state_busy;
        hw->intr_mask = I2C_IC_INTR_MASK_M_TX_EMPTY_BITS// Tx-Empty -> next byte could be send
                    | I2C_IC_INTR_MASK_M_TX_ABRT_BITS   // Tx-Abort -> abort detected
                    | I2C_IC_INTR_MASK_M_RX_FULL_BITS   // Rx-Full -> new byte received
                    | I2C_IC_INTR_MASK_M_STOP_DET_BITS; // Detect Stop -> end of transmission
    }
    return true;
}

/***************************************************************************//**
* @brief Polls and returns the state of the i2c interface
*       (Check timeout if time function is available)
* @return state - the state of i2c interface
*******************************************************************************/
i2c_state_t i2c_drv_poll_state(void)
{
    // If i2c still busy, check tout
    if((state == i2c_state_busy) && (utime_func != NULL))
    {
        ustime_t utime_new = utime_func();
        if(get_diff_ustime(utime_new, utime_start) >= utime_txall)
        {
            // Timeout occured, stop transfer
            i2c_hw_t *hw = i2c_get_hw(I2C_DRV_ID);
            hw->enable = 0;
            hw->intr_mask = 0;
            state = i2c_state_tout;
        }
    }

    return state;
}

/***************************************************************************//**
* @brief Returns the last read i2c data (if there is any received)
* @param ptr_dst [out] pointer to buffer where the received data is copied
* @param dst_max [in] the maximal capacity (in bytes) of ptr_dst buffer
* @return the number of received and copied bytes in ptr_dst
*         if there are more received bytes than dst_max, dst_max is returned
*******************************************************************************/
int i2c_drv_get_rx_data(uint8_t * ptr_dst, int dst_max)
{
    if(state != i2c_state_full)
        return 0;

    if(rd_idx > I2C_DRV_BUFF_LEN)
        rd_idx = I2C_DRV_BUFF_LEN;

    //!!! TODO: Improvement idea: if there is no place in ptr_dst to store 
    // the entire rx_buff, copy dst_max elements, after this don't
    // reset the rd_idx but move the remaining (unread) elements in rx_buff
    // to left. In this case man can read the remaining elements by calling
    // the function once more
    if(rd_idx > dst_max)
        rd_idx = dst_max;

    if(rd_idx > 0)
        memcpy(ptr_dst, rd_buff, rd_idx);
    else
        rd_idx = 0;

    state = i2c_state_idle;
    return rd_idx;
}

/***************************************************************************//**
* @brief Returns the abort source (as mask) of the last transfer or 0ul in case the
*        transfer has finished with success.
* @return 0x00ul - the last transfer finished with success
*         0x01ul - 7-bit address not ACKed by any slave
*         0x02ul - byte 1 of 10Bit address not ACKed by any slave
*         0x04ul - byte 2 of 10Bit address not ACKed by any slave
*         0x08ul - transmitted data not ACKed by addressed slave
*          ...     (see RP2040 documentation for the rest of codes)
*******************************************************************************/
uint32_t i2c_drv_get_abort_source(void)
{
    uint32_t ret = tx_abort_src;
    tx_abort_src = 0ul;
    return ret;
}
