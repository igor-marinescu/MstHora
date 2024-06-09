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

//******************************************************************************
// Includes
//******************************************************************************
#include <stdint.h>
#include <string.h> // memcpy

#include "pico/stdlib.h"

#include "i2c_mem.h"
#include "i2c_drv.h"

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// Global Variables
//******************************************************************************

// Read memory variables
static uint8_t * rd_dst_ptr;    //!< Pointer to buffer where read data is stored
static uint16_t rd_src_addr;    //!< Source Address from where the data is read
static int rd_req_len;          //!< Requested data len in one transfer
static int rd_len;              //!< The length of data to read

// Write memory variables
static uint8_t wr_page[2 + I2C_MEM_PAGE_SIZE];  //!< Buffer used to write 2bytes addr and a PAGE of data
static const uint8_t * wr_src_ptr;  //!< Pointer to buffer from where data is copied
static uint16_t wr_dst_addr;        //!< Destination Address where the data is written
static int wr_req_len;              //!< Requested data len to write in one transfer
static int wr_len;                  //!< The length of data to write

static int abort_retry_idx;         //!< Index of retry in case of an abort response

/***************************************************************************//**
* @brief Init i2c Memory Driver. Must be called in main in init phase
*******************************************************************************/
void i2c_mem_init(void)
{

}

/***************************************************************************//**
* @brief Initiate a memory read request. The function truncates the requested length
*        to the maximal length that the i2c driver can handle in a single transfer.
*        The function doesn't wait until the transfer finishes and exits immediatelly.
*        (The transfer is handled in interrupts).
* @param src_addr [in] - memory source address to read
* @param len [in] - length of data (count of bytes) to read
* @return Length of actual requested data (truncated to maximal i2c transfer length)
          or -1 in case if transfer cannot be initiated (i2c is already busy).
*******************************************************************************/
int i2c_mem_read_request(const uint16_t src_addr, int len)
{
    uint8_t addr_buff[2];

    if(len <= 0)
    {
        I2C_MEM_LOG("i2c_mem_read_request: invalid args\r\n");
        return -1;
    }

    // Read in blocks, every block not bigger than I2C_DRV_BUFF_LEN
    if(len > I2C_DRV_BUFF_LEN)
        len = I2C_DRV_BUFF_LEN;

    I2C_MEM_LOG("i2c_mem_read_request: addr=0x%04x len=%i\r\n", src_addr, len);

    // Set address and initiate read
    addr_buff[0] = (uint8_t) (src_addr >> 8) & 0x0F;
    addr_buff[1] = (uint8_t) (src_addr);
    if(!i2c_drv_transfer_start(I2C_MEM_DEV_ADDR, addr_buff, 2, len))
    {
        I2C_MEM_LOG("i2c_mem_read_request: busy\r\n");
        len = -1;
    }

    return len;
}

/***************************************************************************//**
* @brief Start read memory in non blocking mode (the execution of program is 
*        not blocked and the result of reading must be polled with i2c_mem_read_poll)
* @param dst_ptr [out] - pointer to destination buffer where the read data is copied
* @param src_addr [in] - memory source address
* @param len [in] - length of data (count of bytes) to read
* @return i2c_success - reading process has started check result with i2c_mem_read_poll, 
*         or i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_mem_read_start(uint8_t * dst_ptr, const uint16_t src_addr, int len)
{
    I2C_MEM_LOG("i2c_mem_read_start: addr=0x%04x len=%i\r\n", src_addr, len);

    if((dst_ptr == NULL) || (len <= 0))
    {
        I2C_MEM_LOG("i2c_mem_read_start: i2c_err_argument\r\n");
        return i2c_err_argument;
    }
    
    abort_retry_idx = 0;
    rd_dst_ptr = dst_ptr;
    rd_src_addr = src_addr;
    rd_len = len;

    rd_req_len = i2c_mem_read_request(rd_src_addr, rd_len);
    if(rd_req_len < 0)
    {
        I2C_MEM_LOG("i2c_mem_read_start: i2c_err_busy\r\n");
        return i2c_err_busy;
    }
    if((rd_req_len == 0) || (rd_req_len > rd_len))
    {
        I2C_MEM_LOG("i2c_mem_read_start: i2c_err_unknown\r\n");
        return i2c_err_unknown;
    }

    return i2c_success;
}

/***************************************************************************//**
* @brief Polling the status of read memory in non blocking mode (which has been started
*        with i2c_mem_read_start function)
* @return i2c_success - reading process has finished with success, 
*         i2c_err_busy - reading process is still busy, poll it again later,
*         i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_mem_read_poll(void)
{
    uint32_t tmp32;

    switch(i2c_drv_poll_state())
    {
    case i2c_state_busy:
        return i2c_err_busy;

    case i2c_state_full:
        I2C_MEM_LOG("i2c_mem_read_poll: i2c_state_full\r\n");
        // New data block successfully received, copy it and send next request
        if(i2c_drv_get_rx_data(rd_dst_ptr, rd_len) == rd_req_len)
        {
            rd_len -= rd_req_len;
            rd_dst_ptr += rd_req_len;
            rd_src_addr += (uint16_t) rd_req_len;
            abort_retry_idx = 0;

            // More data to read?
            if(rd_len > 0)
            {
                rd_req_len = i2c_mem_read_request(rd_src_addr, rd_len);
                if((rd_req_len <= 0) || (rd_req_len > rd_len))
                {
                    I2C_MEM_LOG("i2c_mem_read_poll: i2c_err_unknown\r\n");
                    return i2c_err_unknown;
                }

                return i2c_err_busy;
            }
            // None, all read
            I2C_MEM_LOG("i2c_mem_read_poll: i2c_success (rd_len=%i)\r\n", rd_len);
            return i2c_success;
        }
        // Error, received less data than requested
        I2C_MEM_LOG("i2c_mem_read_poll: i2c_err_length\r\n");
        return i2c_err_length;

    case i2c_state_abort:
        tmp32 = i2c_drv_get_abort_source();
        I2C_MEM_LOG("i2c_mem_read_poll: i2c_err_abort (%08lx) %i/%i\r\n", tmp32, abort_retry_idx, I2C_MEM_ABORT_TRIES);
        // If i2c mem is busy writing data?
        // AT24C32 Datasheet: Write Operations:
        // At this time the EEPROM enters an internally-timed write cycle, t_WR (~10..20ms), 
        // to the nonvolatile memory. All inputs are disabled during 
        // this write cycle and the EEPROM will not respond until the write is complete 
        if((tmp32 == 0x01ul) && (abort_retry_idx < I2C_MEM_ABORT_TRIES))
        {
            abort_retry_idx++;
            rd_req_len = i2c_mem_read_request(rd_src_addr, rd_len);
            if((rd_req_len <= 0) || (rd_req_len > rd_len))
            {
                I2C_MEM_LOG("i2c_mem_read_poll: i2c_err_unknown\r\n");
                return i2c_err_unknown;
            }

            return i2c_err_busy;
        }
        return i2c_err_abort;

    case i2c_state_tout:
        I2C_MEM_LOG("i2c_mem_read_poll: i2c_err_tout\r\n");
        return i2c_err_tout;
    
    default:
        break;
    }

    I2C_MEM_LOG("i2c_mem_read_poll: i2c_err_unknown#2\r\n");
    return i2c_err_unknown;
}

/***************************************************************************//**
* @brief Read memory in blocking mode (block program cycle until the transfer finishes)
* @param dst_ptr [out] - pointer to destination buffer where the read data is copied
* @param src_addr [in] - memory source address
* @param len [in] - length of data (count of bytes) to read
* @return i2c_success - reading process has finished with success, 
*         or i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_mem_read_blocking(uint8_t * dst_ptr, const uint16_t src_addr, int len)
{
    i2c_err_t res = i2c_mem_read_start(dst_ptr, src_addr, len);

    if(res != i2c_success)
        return res;

    do 
    {
        tight_loop_contents();
        res = i2c_mem_read_poll();
    } 
    while(res == i2c_err_busy);

    return res;
}

/***************************************************************************//**
* @brief Initiate a memory write request. The function truncates the requested length
*        to the maximal length that the i2c driver can handle in a single write
*        transfer (PAGE size).
*        The function doesn't wait until the transfer finishes and exits immediatelly.
*        (The transfer is handled in interrupts).
* @param dst_addr [in] - memory destination address to write to
* @param src_ptr [in] - pointer to source buffer from where the data is copied
* @param len [in] - length of data (count of bytes) to write
* @return Length of actual requested data to write (truncated to maximal i2c transfer 
         for write: PAGE size)
          or -1 in case if transfer cannot be initiated (i2c is already busy).
*******************************************************************************/
int i2c_mem_write_request(const uint16_t dst_addr, const uint8_t * src_ptr, int len)
{
    if((src_ptr == NULL) || (len <= 0))
    {
        I2C_MEM_LOG("i2c_mem_write_request: invalid args\r\n");
        return -1;
    }

    // Write in blocks, every block not bigger than I2C_MEM_PAGE_SIZE and I2C_DRV_BUFF_LEN
    int len0 = I2C_MEM_PAGE_SIZE - ((int) dst_addr % I2C_MEM_PAGE_SIZE);

    if(len > len0)
        len = len0;

    if(len > I2C_DRV_BUFF_LEN)
        len = I2C_DRV_BUFF_LEN;

    I2C_MEM_LOG("i2c_mem_write_request: addr=0x%04x len=%i\r\n", dst_addr, len);

    // Set address and initiate read
    wr_page[0] = (uint8_t) (dst_addr >> 8) & 0x0F;
    wr_page[1] = (uint8_t) (dst_addr);
    memcpy(&wr_page[2], src_ptr, len);
    if(!i2c_drv_transfer_start(I2C_MEM_DEV_ADDR, wr_page, 2 + len, 0))
    {
        I2C_MEM_LOG("i2c_mem_write_request: busy\r\n");
        len = -1;
    }

    return len;
}

/***************************************************************************//**
* @brief Start write memory in non blocking mode (the execution of program is 
*        not blocked and the status of writing must be polled with i2c_mem_write_poll)
* @param dst_addr [in] - memory destination address to write to
* @param src_ptr [in] - pointer to source buffer from where the data is copied
* @param len [in] - length of data (count of bytes) to write
* @return i2c_success - writing process has started check status with i2c_mem_write_poll, 
*         or i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_mem_write_start(const uint16_t dst_addr, const uint8_t * src_ptr, int len)
{
    I2C_MEM_LOG("i2c_mem_write_start: addr=0x%04x len=%i\r\n", dst_addr, len);

    if((src_ptr == NULL) || (len <= 0))
    {
        I2C_MEM_LOG("i2c_mem_write_start: i2c_err_argument\r\n");
        return i2c_err_argument;
    }
    
    abort_retry_idx = 0;
    wr_src_ptr = src_ptr;
    wr_dst_addr = dst_addr;
    wr_len = len;

    wr_req_len = i2c_mem_write_request(wr_dst_addr, wr_src_ptr, wr_len);
    if(wr_req_len < 0)
    {
        I2C_MEM_LOG("i2c_mem_write_start: i2c_err_busy\r\n");
        return i2c_err_busy;
    }
    if((wr_req_len == 0) || (wr_req_len > wr_len))
    {
        I2C_MEM_LOG("i2c_mem_write_start: i2c_err_unknown\r\n");
        return i2c_err_unknown;
    }

    return i2c_success;
}

/***************************************************************************//**
* @brief Polling the status of write memory in non blocking mode (which has been started
*        with i2c_mem_write_start function)
* @return i2c_success - writing process has finished with success, 
*         i2c_err_busy - writing process is still busy, poll it again later,
*         i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_mem_write_poll(void)
{
    uint32_t tmp32;

    switch(i2c_drv_poll_state())
    {
    case i2c_state_busy:
        return i2c_err_busy;

    case i2c_state_idle:
        I2C_MEM_LOG("i2c_mem_write_poll: i2c_state_idle\r\n");
        // Data block successfully written, send next block
        wr_len -= wr_req_len;
        wr_src_ptr += wr_req_len;
        wr_dst_addr += (uint16_t) wr_req_len;
        abort_retry_idx = 0;

        // More data to write?
        if(wr_len > 0)
        {
            wr_req_len = i2c_mem_write_request(wr_dst_addr, wr_src_ptr, wr_len);
            if((wr_req_len <= 0) || (wr_req_len > wr_len))
            {
                I2C_MEM_LOG("i2c_mem_write_poll: i2c_err_unknown\r\n");
                return i2c_err_unknown;
            }

            return i2c_err_busy;
        }
        // None, all read
        I2C_MEM_LOG("i2c_mem_write_poll: i2c_success (wr_len=%i)\r\n", wr_len);
        return i2c_success;

    case i2c_state_abort:
        tmp32 = i2c_drv_get_abort_source();
        I2C_MEM_LOG("i2c_mem_write_poll: i2c_err_abort (%08lx) %i/%i\r\n", tmp32, abort_retry_idx, I2C_MEM_ABORT_TRIES);
        // If i2c mem is busy writing data?
        // AT24C32 Datasheet: Write Operations:
        // At this time the EEPROM enters an internally-timed write cycle, t_WR (~10..20ms), 
        // to the nonvolatile memory. All inputs are disabled during 
        // this write cycle and the EEPROM will not respond until the write is complete 
        if((tmp32 == 0x01ul) && (abort_retry_idx < I2C_MEM_ABORT_TRIES))
        {
            abort_retry_idx++;
            wr_req_len = i2c_mem_write_request(wr_dst_addr, wr_src_ptr, wr_len);
            if((wr_req_len <= 0) || (wr_req_len > wr_len))
            {
                I2C_MEM_LOG("i2c_mem_write_poll: i2c_err_unknown\r\n");
                return i2c_err_unknown;
            }

            return i2c_err_busy;
        }
        return i2c_err_abort;

    case i2c_state_tout:
        I2C_MEM_LOG("i2c_mem_write_poll: i2c_err_tout\r\n");
        return i2c_err_tout;
    
    default:
        break;
    }

    I2C_MEM_LOG("i2c_mem_write_poll: i2c_err_unknown#2\r\n");
    return i2c_err_unknown;
}

/***************************************************************************//**
* @brief Write memory in blocking mode (block program cycle until the transfer finishes)
* @param dst_addr [in] - memory destination address to write to
* @param src_ptr [in] - pointer to source buffer from where the data is copied
* @param len [in] - length of data (count of bytes) to write
* @return i2c_success - writing process has finished with success 
*         or i2c_err_... in case of error
*******************************************************************************/
i2c_err_t i2c_mem_write_blocking(const uint16_t dst_addr, const uint8_t * src_ptr, int len)
{
    i2c_err_t res = i2c_mem_write_start(dst_addr, src_ptr, len);

    if(res != i2c_success)
        return res;

    do 
    {
        tight_loop_contents();
        res = i2c_mem_write_poll();
    } 
    while(res == i2c_err_busy);

    return res;
}
