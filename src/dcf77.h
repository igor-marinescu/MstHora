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
 * dcf77 - the module scans and decodes the DCF77 signal from DCF77-Radio Board.
 ******************************************************************************/
#ifndef DCF77_H
#define DCF77_H

//******************************************************************************
// Includes
//******************************************************************************
#include "ustime.h"
#include "datetime_utils.h"

#ifdef DCF77_DEBUG
#include DEBUG_INCLUDE
#endif

//******************************************************************************
// Defines
//******************************************************************************
#ifdef DCF77_DEBUG
#define DCF_LOG(...)            DEBUG_PRINTF(__VA_ARGS__)
#define DCF_LOG_TIME(prefix,dt,suffix)  DATETIME_PRINTF_TIME(DEBUG_PRINTF,prefix,dt,suffix)
#define DCF_LOG_DATE(prefix,dt,suffix)  DATETIME_PRINTF_DATE(DEBUG_PRINTF,prefix,dt,suffix)
#else
#define DCF_LOG(...)    
#define DCF_LOG_TIME(prefix,dt,suffix)
#define DCF_LOG_DATE(prefix,dt,suffix)
#endif

#define DCF_IN_PIN      13          // Pin index where input DCF77 signal is connected

#define DCF_BIT0_MIN    50000L      // Minimal time [us] of signal for valid 0-bit
#define DCF_BIT0_MAX    175000L     // Maximal time [us] of signal for valid 0-bit
#define DCF_BIT1_MIN    175001L     // Minimal time [us] of signal for valid 1-bit
#define DCF_BIT1_MAX    350000L     // Maximal time [us] of signal for valid 1-bit
#define DCF_SYNC_MIN    1500000L    // Minimal time [us] of sync-signal

#define DCF_T_59SEC     59000000L
#define DCF_T_1SEC      1000000L

// After how many consecutive successful time/date decodes, the value is considered valid
#define DCF_VALID_DATETIME_CNT  3

// Signal edges
#define DCF_EDGE_RAISING    0x01    // __|-- Raising Edge detected 
#define DCF_EDGE_FALLING    0x02    // --|__ Falling Edge detected

// Measure signal quality
#define DCF_Q_FILTER        10      // Median filter size (window size)

// Bit Values
typedef enum {
    dcf_bitval_none = 0,    // Bit is not defined (has invalid value)
    dcf_bitval_false = -1,  // Bit is defined as False/Zero
    dcf_bitval_true = 1     // Bit is defined as True/One
} dcf_bitval_t;

typedef struct {
    ustime_t start;     // Bit start-time
    ustime_t end;       // Bit end-time
    ustime_t len;       // Bit length
    uint8_t edge;       // Signal edges
    dcf_bitval_t val;   // Bit value
} dcf_bit_t;

#define DCF_CLEAR_BIT(b)    memset(&(b), 0, sizeof(b))
#define DCF_COPY_BIT(b, c)  memcpy(&(b), &(c), sizeof(b));

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init DCF77 Module. Must be called in main in init phase
void dcf_init(void);

// DCF77 polling function. Must be called every program cycle
bool dcf_poll(const ustime_t sys_ustime);

// Get signal quality 0..100%
int dcf_get_quality(void);

// Return last detected datetime if valid
datetime_t * dcf_get_datetime(void);

//******************************************************************************
#endif /* DCF77_H */
