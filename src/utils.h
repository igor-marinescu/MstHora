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
 * utils - the implementation of a set of utility functions.
 ******************************************************************************/
#ifndef UTILS_H
#define UTILS_H

//******************************************************************************
// Includes
//******************************************************************************

//******************************************************************************
// Defines
//******************************************************************************
#define HB_FROM_WORD(x)   (uint8_t)(((uint16_t) (x)) >> 8)
#define LB_FROM_WORD(x)   (uint8_t)(((uint16_t) (x)))

//******************************************************************************
// Typedefs
//******************************************************************************

// Median (modified) filter
typedef struct {
    int * buff;     // Pointer to data buffer containing data
    unsigned int size;  // Size of the data buffer in elements
    long sum;       // Sum of all elements in data buffer
    bool init_flag;     // Flag inidcates the buffer contain valid data
} m_filter_int_t;

//******************************************************************************
// Exported Functions
//******************************************************************************

// Extract integer value from a text-string
int utils_extract_int(int * out_val, const char * in_str, int in_max_len);

// Convert a string to a decimal integer
bool utils_get_int_dec(int * out_val, const char * in_str, int in_max_len);

// Convert a string representing a hexadecimal integer to integer
bool utils_get_int_hex(int * out_val, const char * in_str, int in_max_len);

// Convert a string representing an integer (decimal or hexadecimal form)
bool utils_get_int(int * out_val, const char * in_str, int in_max_len);

// Convert a string to a decimal long-integer
bool utils_get_long_dec(long * out_val, const char * in_str, int in_max_len);

// Convert a string representing a hexadecimal long-integer to long-integer
bool utils_get_long_hex(long * out_val, const char * in_str, int in_max_len);

// Convert a string representing a long-integer (decimal or hexadecimal form) 
bool utils_get_long(long * out_val, const char * in_str, int in_max_len);

// Convert BCD to INT8
int8_t bcd_to_int8(uint8_t in_bcd);

// Convert INT8 to BCD
uint8_t int8_to_bcd(uint8_t in_u8);

// Init uint8_t median filter structure
void m_filter_int_init(m_filter_int_t * filter, int * buff, unsigned int size);

// Add val to filter and calculate filtered value
int m_filter_int_add_val(m_filter_int_t * filter, int val);

//******************************************************************************
#endif /* UTILS_H */
