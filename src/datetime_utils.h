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
 * datetime_utils - the module implements useful functions to work with the
 * datetime_t data structure.
 ******************************************************************************/
#ifndef DATETIME_UTILS_H
#define DATETIME_UTILS_H

//******************************************************************************
// Includes
//******************************************************************************
#include "pico/types.h"

//******************************************************************************
// Defines
//******************************************************************************

/*
// datetime_t defined in src/common/pico_base/include/pico/types.h
typedef struct {
    int16_t year;    ///< 0..4095
    int8_t month;    ///< 1..12, 1 is January
    int8_t day;      ///< 1..28,29,30,31 depending on month
    int8_t dotw;     ///< 0..6, 0 is Sunday
    int8_t hour;     ///< 0..23
    int8_t min;      ///< 0..59
    int8_t sec;      ///< 0..59
} datetime_t;
*/

#define DATETIME_PRINTF_TIME(func,prefix,dt,suffix) \
        (func)(prefix "%2.2i:%2.2i:%2.2i" suffix, (dt).hour, (dt).min, (dt).sec)
#define DATETIME_PRINTF_DATE(func,prefix,dt,suffix) \
        (func)(prefix "%2.2i.%2.2i.%4.4i (%i)" suffix, (dt).day, (dt).month, (dt).year, (dt).dotw)

//******************************************************************************
// Exported Functions
//******************************************************************************

// Convert Text representing time in format "hh:mm:ss" to datetime_t
int datetime_time_from_text(datetime_t * out, const char * in_str, int in_max_len);

// Convert Text representing date in format "DD.MM.YY" to datetime_t
int datetime_date_from_text(datetime_t * out, const char * in_str, int in_max_len);

// Check if in_time contains a valid time
bool datetime_is_valid_time(const datetime_t * in_time);

// Check if in_date contains a valid date
bool datetime_is_valid_date(const datetime_t * in_date);

// Check if datetime contains valid values
bool datetime_is_valid(const datetime_t * in_datetime);

// Clear time structure (set to: 00:00:00)
void datetime_clear_time(datetime_t * ptr_time);

// Copy source in_time to destination out_time
void datetime_copy_time(datetime_t * out_time, const datetime_t * in_time);

// Clear date structure (set to 01.01.2000)
void datetime_clear_date(datetime_t * ptr_date);

// Copy source in_date to destination out_date
void datetime_copy_date(datetime_t * out_date, const datetime_t * in_date);

// Clear datetime variable (set to: 00:00:00 01.01.2000)
void datetime_clear(datetime_t * dt);

// Copy source datetime to destination datetime
void datetime_copy(datetime_t * out_dt, const datetime_t * in_dt);

// Add seconds to time
int datetime_add_sec(datetime_t * time, int sec);

// Compare two time variables
int datetime_time_compare(const datetime_t * time1, const datetime_t * time2);

// Compare two date variables
int datetime_date_compare(const datetime_t * date1, const datetime_t * date2);

// Compare two datetime variables
int datetime_compare(const datetime_t * dt1, const datetime_t * dt2);

// Check if two datetime variables are equal
bool datetime_is_equal(const datetime_t * dt1, const datetime_t * dt2);

// Convert a time variable to seconds
int datetime_time_to_sec(const datetime_t * time);

// Get difference in seconds between time1 and time2
int datetime_time_diff(const datetime_t * time1, const datetime_t * time2);

//******************************************************************************
#endif /* DATETIME_UTILS_H */
