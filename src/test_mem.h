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
#ifndef TEST_MEM_H
#define TEST_MEM_H

//******************************************************************************
// Includes
//******************************************************************************

//******************************************************************************
// Defines
//******************************************************************************
//#define TEST_MEM_PRINTF(...)            io_printf(__VA_ARGS__)
//#define TEST_MEM_DUMP(buff, len, addr)  io_dump((buff), (len), (addr))

// Test Buffer size in bytes
#define TEST_MEM_BUFF_SIZE  256

// Memory size in bytes
#define TEST_MEM_SIZE   4096

// Block Data Pattern Type
typedef enum {
    test_mem_data_pattern_zero = 0, // Pattern=0, 00 00 00 00 00 00 00 00... <- all nulls
    test_mem_data_pattern_seq1,     // Pattern=1: 00 01 02 03 04 05 06 07... <- sequence of bytes
    test_mem_data_pattern_seq2,     // Pattern=2: 00 00 02 00 04 00 06 00... <- sequence of words
    test_mem_data_pattern_fill,     // Pattern=3: FF FF FF FF FF FF FF FF... <- all FFs
} test_mem_data_pattern_t;

// Block Size Pattern Type
typedef enum {
    test_mem_size_pattern_max = 0,  // Pattern=0, use maximal possible block size (TEST_MEM_BUFF_SIZE)
    test_mem_size_pattern_inc,      // Pattern=1: start with 1 and with every transfer increment block size
    test_mem_size_pattern_dec,      // Pattern=2: start with max and with every transfer decrement block size
    test_mem_size_pattern_mix,      // Pattern=3: 1, max, 2, max-1, 3, max-2, 4, max-3,... max/2
} test_mem_size_pattern_t;

// Operation
typedef enum {
    test_mem_op_none = 0,
    test_mem_op_write,
    test_mem_op_read,
    test_mem_op_check,
    test_mem_op_auto
} test_mem_op_t;

// Read/Write request
typedef struct {
    test_mem_op_t op;
    int addr;
    int len;
    test_mem_size_pattern_t size_pattern;
    test_mem_data_pattern_t data_pattern;
} test_mem_req_t;

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init test_mem module
void test_mem_init(void);

// Test memory state-machine
bool test_mem_poll(void);

// Return true if memory test finished with error
bool test_mem_is_error(void);

// Request to test memory
void test_mem_req(const test_mem_req_t * req_ptr);

//******************************************************************************
#endif /* TEST_MEM_H */
