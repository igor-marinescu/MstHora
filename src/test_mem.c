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
 * test_mem - the module to test the EEPROM memory.
 ******************************************************************************/

//******************************************************************************
// Includes
//******************************************************************************
#include <stdint.h>
#include <string.h> // memcpy
#include <stdio.h>

#include "pico/stdlib.h"

#include "test_mem.h"
#include "i2c_mem.h"
#include "i2c_drv.h"
#include "in_out.h"
#include "utils.h"

//******************************************************************************
// Typedefs
//******************************************************************************

// State machine
typedef enum {
    state_none = 0,
    state_wr_block,
    state_wr_poll,
    state_rd_block,
    state_rd_poll,
    state_error
} state_t;

// Read/Write structure
typedef struct {
    uint8_t cfg;
    int addr;
    int addr_end;
    int block_size;
    test_mem_size_pattern_t size_pattern;
    test_mem_data_pattern_t data_pattern;
    int data_pattern_idx;
} rw_t;

// Command configuration
#define TEST_MEM_CFG_NONE           0x00
#define TEST_MEM_CFG_RD_DISPLAY     0x01
#define TEST_MEM_CFG_RD_CHECK       0x02

//******************************************************************************
// Global Variables
//******************************************************************************

static state_t state = state_none;
static i2c_err_t i2c_err;

static rw_t rw;     // Read/Write structure
static test_mem_req_t req;  // Request structure

static uint8_t wr_buff[TEST_MEM_BUFF_SIZE]; // Data buffer used for write 
static uint8_t rd_buff[TEST_MEM_BUFF_SIZE]; // Data buffer used for read 
static uint8_t ck_buff[TEST_MEM_BUFF_SIZE]; // Data buffer used for checking data (TEST_MEM_CFG_RD_CHECK)

// Auto requests, used when executing: test_mem auto
static const test_mem_req_t auto_req_list[] = {
//   <------- op ------>|<-addr->|<---- len --->|<----- size_pattern ----->|<----- data_pattern ----->|
    { test_mem_op_write, 0x0000, TEST_MEM_SIZE, test_mem_size_pattern_max, test_mem_data_pattern_zero },
    { test_mem_op_check, 0x0000, TEST_MEM_SIZE, test_mem_size_pattern_max, test_mem_data_pattern_zero },
    { test_mem_op_write, 0x0000, TEST_MEM_SIZE, test_mem_size_pattern_inc, test_mem_data_pattern_seq1 },
    { test_mem_op_check, 0x0000, TEST_MEM_SIZE, test_mem_size_pattern_dec, test_mem_data_pattern_seq1 },
    { test_mem_op_write, 0x0000, TEST_MEM_SIZE, test_mem_size_pattern_mix, test_mem_data_pattern_seq2 },
    //{ test_mem_op_write, 0x0ABC, 1, test_mem_size_pattern_max, test_mem_data_pattern_zero },
    { test_mem_op_check, 0x0000, TEST_MEM_SIZE, test_mem_size_pattern_inc, test_mem_data_pattern_seq2 },
    { test_mem_op_write, 0x0000, TEST_MEM_SIZE, test_mem_size_pattern_dec, test_mem_data_pattern_fill },
    { test_mem_op_check, 0x0000, TEST_MEM_SIZE, test_mem_size_pattern_mix, test_mem_data_pattern_fill },
    { test_mem_op_write, 0x0000, TEST_MEM_SIZE, test_mem_size_pattern_max, test_mem_data_pattern_fill },
    { test_mem_op_check, 0x0000, TEST_MEM_SIZE, test_mem_size_pattern_max, test_mem_data_pattern_fill },
};

static const int auto_req_cnt = sizeof(auto_req_list) / sizeof(test_mem_req_t);
static int auto_req_idx = -1;

/***************************************************************************//**
* @brief Init test_mem module. Must be called in init-phase.
*******************************************************************************/
void test_mem_init(void)
{
    // Not used
}

/***************************************************************************//**
* @brief Init and populate read/write structure based on read/write request.
* @param req_ptr [in] Pointer to request to initialize the read/write structure
* @return new state of the state-machine (based on request)
*******************************************************************************/
static state_t init_rw(const test_mem_req_t * req_ptr)
{
    state_t state_new = state_none;

    switch(req_ptr->op)
    {
        case test_mem_op_write:
            rw.cfg = TEST_MEM_CFG_NONE;
            state_new = state_wr_block;
            break;

        case test_mem_op_read:
            rw.cfg = TEST_MEM_CFG_RD_DISPLAY;
            state_new = state_rd_block;
            break;

        case test_mem_op_check:
            rw.cfg = TEST_MEM_CFG_RD_CHECK;
            state_new = state_rd_block;
            break;

        default:
            break;
    }   

    if(state_new != state_none)
    {
        rw.addr = req_ptr->addr;
        rw.addr_end = req_ptr->addr + req_ptr->len;
        rw.size_pattern = req_ptr->size_pattern;
        rw.data_pattern = req_ptr->data_pattern;
        rw.data_pattern_idx = 0;
        rw.block_size = 0;
        // Reset last error        
        i2c_err = i2c_success;
    }

    return state_new;
}

/***************************************************************************//**
* @brief Get the next request from the array of automatic requests.
* @return Pointer to the next atomatic requests 
*       or NULL in case there are no more requests
*******************************************************************************/
static const test_mem_req_t * next_auto_req(void)
{
    if(auto_req_idx < 0)
        return NULL;

    while(auto_req_idx < auto_req_cnt)
    {
        if(auto_req_list[auto_req_idx].op != test_mem_op_none)
            return &auto_req_list[auto_req_idx];

        auto_req_idx++;
    }

    return NULL;
}

/***************************************************************************//**
* @brief Populate the next block size field in the read/write field based on size pattern.
* @param rw_ptr [in/out] pointer to read/write structure
* @param capacity [in] maximal capacity of the read/write buffer (rd_buff, wr_buff, ...)
*******************************************************************************/
static void size_pattern(rw_t * rw_ptr, int capacity)
{
    int new_size = capacity;

    switch(rw_ptr->size_pattern)
    {
        case test_mem_size_pattern_inc:
            new_size = rw_ptr->block_size + 1;
            if(new_size > capacity)
                new_size = 1;
            break;

        case test_mem_size_pattern_dec:
            new_size = rw_ptr->block_size - 1;
            if(new_size < 1)
                new_size = capacity;
            break;

        case test_mem_size_pattern_mix:
            if(rw_ptr->block_size < (capacity / 2))
                new_size = capacity - rw_ptr->block_size - 1;
            else
                new_size = capacity - rw_ptr->block_size + 1;
            break;

        default:
            break;
    }

    if(new_size > (rw_ptr->addr_end - rw_ptr->addr))
        new_size = (rw_ptr->addr_end - rw_ptr->addr);
    if(new_size > capacity)
        new_size = capacity;
    if(new_size < 1)
        new_size = 1;

    rw_ptr->block_size = new_size;
}

/***************************************************************************//**
* @brief Populate a buffer with a pattern of bytes:
* @param buff_ptr [out] pointer to buffer to be populated with pattern
* @param buff_len [in] maximal length of input buffer in bytes
* @param rw_ptr [in/out] pointer to read/write structure containing data pattern definition
*******************************************************************************/
static void data_pattern(uint8_t * buff_ptr, int buff_len, rw_t * rw_ptr)
{
    int len = rw_ptr->block_size;
    if(len > buff_len)
        len = buff_len;

    int len2 = len;
    int pidx = rw_ptr->data_pattern_idx;

    switch(rw_ptr->data_pattern)
    {
        case test_mem_data_pattern_seq1: // Pattern: 00 01 02 03...<len>
            for(int i = 0; i < len; i++, pidx++)
                buff_ptr[i] = (uint8_t) pidx;
            break;

        case test_mem_data_pattern_seq2: // Pattern: 00 00 01 00 02 00...LL HH
            // High-Byte from previous Word?
            if(((pidx % 2) != 0) && (len2 > 0))
            {
                *buff_ptr = HB_FROM_WORD(pidx); buff_ptr++;
                pidx++;
                len2--;
            }

            // len2/2 Words
            while(len2 > 1)
            {
                *buff_ptr = LB_FROM_WORD(pidx); buff_ptr++;
                *buff_ptr = HB_FROM_WORD(pidx); buff_ptr++;
                pidx += 2;
                len2 -= 2;
            }

            // Low-Byte from next Word?
            if(len2 > 0)
            {
                *buff_ptr = LB_FROM_WORD(pidx);
                pidx++;
            }
            break;

        case test_mem_data_pattern_fill: // Pattern: FF FF FF FF...FF
            memset(buff_ptr, 0xFF, len);
            break;

        default: // Pattern 00 00 00 00...00
            memset(buff_ptr, 0, len);
            break;
    }

    rw_ptr->data_pattern_idx = pidx;
    rw_ptr->block_size = len;
}

/***************************************************************************//**
* @brief Check if a buffer matches a pattern of bytes
* @param buff_ptr [in] pointer to buffer to be checked against the pattern
* @param buff_len [in] length of input buffer in bytes
* @param rw_ptr [in/out] pointer to read/write structure containing data pattern definition
* @return -1 in case of success (buffer matches the pattern of bytes)
*         > -1 returns the offset (index) in buffer where the pattern doesn't match
*******************************************************************************/
static int data_check(uint8_t * buff_ptr, int buff_len, rw_t * rw_ptr)
{
    int len = buff_len;
    if(len > sizeof(ck_buff))
        len = sizeof(ck_buff);
    
    // Populate check buffer with test data
    data_pattern(ck_buff, sizeof(ck_buff), rw_ptr);
    
    // Compare buff_ptr against check buffer
    for(int i = 0; i < len; i++)
    {
        if(buff_ptr[i] != ck_buff[i])
            return i;
    }

    // Is the input buffer bigger than check buffer?
    // we cannot compare the rest
    if(buff_len > sizeof(ck_buff))
        return sizeof(ck_buff);

    return -1;
}

/***************************************************************************//**
* @brief Write the next block of data (wr_buff)
* @return new state of the state machine
*******************************************************************************/
static state_t wr_block(void)
{
    if(rw.addr >= rw.addr_end)
    {
        io_printf("test_mem: Write finished\r\n");
        return state_none;
    }

    size_pattern(&rw, sizeof(wr_buff));
    data_pattern(wr_buff, sizeof(wr_buff), &rw);

    io_printf("test_mem: wr 0x%04x len=%i\r\n", rw.addr, rw.block_size);
    i2c_err = i2c_mem_write_start((uint16_t) rw.addr, wr_buff, rw.block_size);
    if(i2c_err != i2c_success)
    {
        io_printf("test_mem: Error i2c_mem_write_start=%i\r\n", (int) i2c_err);
        return state_error;
    }
    return state_wr_poll;
}

/***************************************************************************//**
* @brief Poll the status of previous write
* @return new state of the state machine
*******************************************************************************/
static state_t wr_poll(void)
{
    i2c_err = i2c_mem_write_poll();
    if(i2c_err == i2c_success)
    {
        rw.addr += rw.block_size;
        return state_wr_block;
    }
    else if(i2c_err != i2c_err_busy)
    {
        io_printf("test_mem: Error i2c_mem_write_poll=%i\r\n", (int) i2c_err);
        return state_error;
    }
    return state_wr_poll;
}

/***************************************************************************//**
* @brief Read the next block of data (rd_buff)
* @return new state of the state machine
*******************************************************************************/
static state_t rd_block(void)
{
    if(rw.addr >= rw.addr_end)
    {
        io_printf("test_mem: Read finished\r\n");
        return state_none;
    }

    size_pattern(&rw, sizeof(rd_buff));

    io_printf("test_mem: rd 0x%04x len=%i\r\n", rw.addr, rw.block_size);
    i2c_err = i2c_mem_read_start(rd_buff, (uint16_t) rw.addr, rw.block_size);
    if(i2c_err != i2c_success)
    {
        io_printf("test_mem: Error i2c_mem_read_start=%i\r\n", (int) i2c_err);
        return state_error;
    }
    
    return state_rd_poll;
}

/***************************************************************************//**
* @brief Poll the status of previous read
* @return new state of the state machine
*******************************************************************************/
static state_t rd_poll(void)
{
    i2c_err = i2c_mem_read_poll();
    if(i2c_err == i2c_success)
    {
        if(rw.cfg & TEST_MEM_CFG_RD_DISPLAY)
            io_dump(rd_buff, rw.block_size, rw.addr);
        if(rw.cfg & TEST_MEM_CFG_RD_CHECK)
        {
            int idx = data_check(rd_buff, rw.block_size, &rw);
            if(idx > -1)
            {
                io_printf("test_mem: Error inconsistency @ 0x%04x (expected: 0x%02x)\r\n", rw.addr + idx, ck_buff[idx]); //!!!

                io_dump(rd_buff, rw.block_size, rw.addr);
                io_printf("----------------------\r\n");
                io_dump(ck_buff, rw.block_size, rw.addr);

                i2c_err = i2c_err_format;
                return state_error;
            }
        }
        rw.addr += rw.block_size;
        return state_rd_block;
    }
    else if(i2c_err != i2c_err_busy)
    {
        io_printf("test_mem: Error i2c_mem_read_poll=%i\r\n", (int) i2c_err);
        return state_error;
    }
    return state_rd_poll;
}

/***************************************************************************//**
* @brief Test memory state-machine. Must be called every program-cycle.
* @return busy state: true - memory test still busy, must be called next cycle
*                     false - memory test finished or idle
*******************************************************************************/
bool test_mem_poll(void)
{
    switch(state)
    {
        case state_wr_block:
            state = wr_block();
            break;

        case state_wr_poll:
            state = wr_poll();
            break;

        case state_rd_block:
            state = rd_block();
            break;

        case state_rd_poll:
            state = rd_poll();
            break;

        case state_error:
            auto_req_idx = -1;
            state = state_none;
            break;

        default:
            state = state_none;
            break;
    }

    // Idle?
    if(state == state_none)
    {
        // Check for new request (if auto test is not running)
        if(auto_req_idx < 0)
        {
            if(req.op != test_mem_op_none)
            {
                if(req.op == test_mem_op_auto)
                    auto_req_idx = 0;
                else
                    state = init_rw(&req);
                req.op = test_mem_op_none;
            }
        }
        // Auto request if active (auto_req_idx > -1)
        if(auto_req_idx > -1)
        {
            const test_mem_req_t * req_ptr = next_auto_req();
            if(req_ptr != NULL)
                state = init_rw(req_ptr);
            if(state != state_none)
                auto_req_idx++;
            else
                auto_req_idx = -1;
        }
    }

    return (state != state_none);
}

/***************************************************************************//**
* @brief Return true if memory test finished with error
* @return true - memory test finished with error
*        false - memory test idle, still running or finished with success
*******************************************************************************/
bool test_mem_is_error(void)
{
    return (i2c_err != i2c_success);
}

/***************************************************************************//**
* @brief Generate and display the memory test-pattern for a read/write
* @param req_ptr [in] pointer to request data
*******************************************************************************/
/*
void test_mem_gen_pattern(const test_mem_req_t * req_ptr)
{
    rw_t rw;
    init_rw(&rw, req_ptr);

    while(rw.addr < rw.addr_end)
    {
        size_pattern(&rw, sizeof(ck_buff));
        data_pattern(ck_buff, sizeof(ck_buff), &rw);

        io_printf("addr = 0x%04x\r\n", rw.addr);
        io_printf("a_end = 0x%04x\r\n", rw.addr_end);
        io_printf("b_size = %i\r\n", rw.block_size);
        io_printf("d_pidx = %i\r\n", rw.data_pattern_idx);

        io_dump(ck_buff, rw.block_size, rw.addr);

        rw.addr += rw.block_size;
    }
}
*/

/***************************************************************************//**
* @brief Test memory request
* @param req_ptr [in] pointer to request data
*******************************************************************************/
void test_mem_req(const test_mem_req_t * req_ptr)
{
    //io_printf("test_mem_req: op=%i addr=%i len=%i\r\n", (int) req_ptr->op, (int) req_ptr->addr, (int) req_ptr->len);
    memcpy(&req, req_ptr, sizeof(req));
}
