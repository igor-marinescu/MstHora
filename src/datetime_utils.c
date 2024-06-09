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

//******************************************************************************
// Includes
//******************************************************************************
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h> // memcpy

#include "datetime_utils.h"
#include "utils.h"

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// Global Variables
//******************************************************************************

/***************************************************************************//**
* @brief Convert Text representing time in format "hh:mm:ss" to datetime_t
* @param out [out] pointer to datetime_t where the converted date and time is stored
* @param in_str [in] text containing the time
* @param in_max_len [in] maximal length of the text
* @return End position in in_str of the extracted time or -1 in case of error
*******************************************************************************/
int datetime_time_from_text(datetime_t * out, const char * in_str, int in_max_len)
{
    int val, res, pos;
    datetime_t dt;

    // Extract hours (valid values: 0..23)
    res = utils_extract_int(&val, in_str, in_max_len);
    if((res < 0) || (val < 0) || (val > 23))
        return -1;

    dt.hour = (int8_t) val;

    // Check ':' symbol
    if((res >= in_max_len) || (in_str[res] != ':'))
        return -1;

    res++;
    in_max_len -= res;
    in_str += res;
    pos = res;

    // Extract minutes (valid values: 0..59)
    res = utils_extract_int(&val, in_str, in_max_len);
    if((res < 0) || (val < 0) || (val > 59))
        return -1;

    dt.min = (int8_t) val;

    // Check ':' symbol
    if((res >= in_max_len) || (in_str[res] != ':'))
        return -1;

    res++;
    in_max_len -= res;
    in_str += res;
    pos += res;

    // Extract seconds (valid values: 0..59)
    res = utils_extract_int(&val, in_str, in_max_len);
    if((res < 0) || (val < 0) || (val > 59))
        return -1;

    dt.sec = (int8_t) val;
    in_max_len -= res;
    pos += res;

    // If there are symbols left in string, check the next symbol
    // only space (' ', '\t') or null ('\0') is allowed to follow
    if((in_max_len > 0) && (in_str[res] != ' ') && (in_str[res] != '\t') && (in_str[res] != '\0'))
        return -1;

    if(out != NULL)
    {
        datetime_copy_time(out, &dt);
    }

    return pos;
}

/***************************************************************************//**
* @brief Convert Text representing date in format "DD.MM.YY" to datetime_t
* @param out [out] pointer to datetime_t variable where the converted date is stored
* @param in_str [in] text containing the date
* @param in_max_len [in] maximal length of the text
* @return End position in in_str of the extracted date or -1 in case of error
*******************************************************************************/
int datetime_date_from_text(datetime_t * out, const char * in_str, int in_max_len)
{
    int val, res, pos;
    datetime_t dt;

    // Extract day (valid values: 1..31)
    res = utils_extract_int(&val, in_str, in_max_len);
    if((res < 0) || (val < 1) || (val > 31))
        return -1;

    dt.day = (int8_t) val;

    // Check '.' symbol
    if((res >= in_max_len) || (in_str[res] != '.'))
        return -1;

    res++;
    in_max_len -= res;
    in_str += res;
    pos = res;

    // Extract month (valid values: 1..12)
    res = utils_extract_int(&val, in_str, in_max_len);
    if((res < 0) || (val < 1) || (val > 12))
        return -1;

    dt.month = (int8_t) val;

    // Check '.' symbol
    if((res >= in_max_len) || (in_str[res] != '.'))
        return -1;

    res++;
    in_max_len -= res;
    in_str += res;
    pos += res;

    // Extract year (valid values 0..4095)
    res = utils_extract_int(&val, in_str, in_max_len);
    if((res < 0) || (val < 0) || (val > 4095))
        return -1;

    dt.year = (int16_t) val;
    in_max_len -= res;
    pos += res;

    // If there are symbols left in string, check the next symbol
    // only space (' ', '\t') or null ('\0') is allowed to follow
    if((in_max_len > 0) && (in_str[res] != ' ') && (in_str[res] != '\t') && (in_str[res] != '\0'))
        return -1;

    if(out != NULL)
    {
        datetime_copy_date(out, &dt);
    }

    return pos;
}

/***************************************************************************//**
* @brief Check if in_time contains a valid time
* @param in_time [in] pointer to time variable to check
* @return true - in_time contains a valid time, or false in case if not
*******************************************************************************/
bool datetime_is_valid_time(const datetime_t * in_time)
{
    if(!in_time)
        return false;
    if((in_time->sec < 0) || (in_time->sec > 59))
        return false;
    if((in_time->min < 0) || (in_time->min > 59)) 
        return false;
    if((in_time->hour < 0) || (in_time->hour > 23))
        return false;
    return true;
}

/***************************************************************************//**
* @brief Check if in_date contains a valid date
* @param in_date [in] pointer to date variable to check
* @return true - in_date contains a valid date, or false in case if not
*******************************************************************************/
bool datetime_is_valid_date(const datetime_t * in_date)
{
    if(!in_date)
        return false;
    if((in_date->day < 1) || (in_date->day > 31))
        return false;
    if((in_date->month < 1) || (in_date->month > 12))
        return false;
    if((in_date->year < 0) || (in_date->year > 4095))
        return false;
    if((in_date->dotw < 0) || (in_date->dotw > 6))
        return false;
    return true;
}

/***************************************************************************//**
* @brief Check if datetime contains valid values
* @param in_datetime [in] pointer to datetime variable to check
* @return true - in_datetime contains valid data, or false in case if not
*******************************************************************************/
bool datetime_is_valid(const datetime_t * in_datetime)
{
    return (datetime_is_valid_time(in_datetime) && datetime_is_valid_date(in_datetime));
}

/***************************************************************************//**
* @brief Clear time (set to: 00:00:00)
* @param ptr_time [in/out] pointer to datetime structure to clear time
*******************************************************************************/
void datetime_clear_time(datetime_t * ptr_time)
{
    if(ptr_time)
    {
        ptr_time->hour = 0;
        ptr_time->min = 0;
        ptr_time->sec = 0;
    }
}

/***************************************************************************//**
* @brief Copy source in_time to destination out_time
* @param in_time [in] pointer to source time
* @param out_time [out] pointer to destination time
*******************************************************************************/
void datetime_copy_time(datetime_t * out_time, const datetime_t * in_time)
{
    if(out_time && in_time)
    {
        out_time->hour = in_time->hour;
        out_time->min = in_time->min;
        out_time->sec = in_time->sec;
    }
}

/***************************************************************************//**
* @brief Clear date structure (set to 01.01.2000)
* @param ptr_date [in/out] pointer to date structure to clear
*******************************************************************************/
void datetime_clear_date(datetime_t * ptr_date)
{
    if(ptr_date)
    {
        ptr_date->dotw = 0;
        ptr_date->day = 1;
        ptr_date->month = 1;
        ptr_date->year = 2000;
    }
}

/***************************************************************************//**
* @brief Copy source in_date to destination out_date
* @param in_date [in] pointer to source date
* @param out_date [out] pointer to destination date
*******************************************************************************/
void datetime_copy_date(datetime_t * out_date, const datetime_t * in_date)
{
    if(out_date && in_date)
    {
        out_date->dotw = in_date->dotw;
        out_date->day = in_date->day;
        out_date->month = in_date->month;
        out_date->year = in_date->year;
    }
}

/***************************************************************************//**
* @brief Clear datetime structure (set to: 00:00:00 01.01.2000)
* @param dt [in/out] pointer to datetime structure to clear
*******************************************************************************/
void datetime_clear(datetime_t * dt)
{
    datetime_clear_time(dt);
    datetime_clear_date(dt);
}

/***************************************************************************//**
* @brief Copy source datetime to destination datetime
* @param in_dt [in] pointer to source datetime
* @param out_dt [out] pointer to destination datetime
*******************************************************************************/
void datetime_copy(datetime_t * out_dt, const datetime_t * in_dt)
{
    if(out_dt && in_dt)
    {
        memcpy(out_dt, in_dt, sizeof(datetime_t));
    }
}

/***************************************************************************//**
* @brief Add seconds to time (doesn't work with negative values for sec)
* @param time [in/out] pointer to time variable to add seconds to
* @param sec [in] secnds to add to time (only positive values)
* @return days
*******************************************************************************/
int datetime_add_sec(datetime_t * time, int sec)
{
    if(!time)
        return 0;

    int sec_new = sec + (int) time->sec;
    time->sec = (int8_t)(sec_new % 60);

    int min_new = ((int) time->min) + (sec_new / 60);
    time->min = (int8_t)(min_new % 60);

    int hr_new = ((int) time->hour) + (min_new / 60);
    time->hour = (int8_t)(hr_new % 24);

    return (hr_new / 24);
}

/***************************************************************************//**
* @brief Compare two time variables
* @param time1 [in] time variable 1 to compare
* @param time2 [in] time variable 2 to compare
* @return  0: time1 == time2
*         -1: time1 < time 2
*         +1: time1 > time 2
*******************************************************************************/
int datetime_time_compare(const datetime_t * time1, const datetime_t * time2)
{
    if(time1->hour < time2->hour)
        return -1;
    else if(time1->hour > time2->hour)
        return 1;

    if(time1->min < time2->min)
        return -1;
    else if(time1->min > time2->min)
        return 1;

    if(time1->sec < time2->sec)
        return -1;
    else if(time1->sec > time2->sec)
        return 1;

    return 0;
}

/***************************************************************************//**
* @brief Compare two date variables
* @param date1 [in] date variable 1
* @param date2 [in] date variable 2
* @return  0: date1 == date2
*         -1: date1 < date 2
*         +1: date1 > date 2
*******************************************************************************/
int datetime_date_compare(const datetime_t * date1, const datetime_t * date2)
{
    if(date1->year < date2->year)
        return -1;
    else if(date1->year > date2->year)
        return 1;

    if(date1->month < date2->month)
        return -1;
    else if(date1->month > date2->month)
        return 1;

    if(date1->day < date2->day)
        return -1;
    else if(date1->day > date2->day)
        return 1;

    return 0;
}

/***************************************************************************//**
* @brief Compare two datetime variables
* @param dt1 [in] pointer to datetime variable 1
* @param dt2 [in] pointer to datetime variable 2
* @return  0: datetime1 == datetime2
*         -1: datetime 1 < datetime 2
*         +1: datetime 1 > datetime 2
*******************************************************************************/
int datetime_compare(const datetime_t * dt1, const datetime_t * dt2)
{
    int res = datetime_date_compare(dt1, dt2);
    if(!res)
        return datetime_time_compare(dt1, dt2);
    return res;
}

/***************************************************************************//**
* @brief Check if two datetime variables are equal
* @param dt1 [in] pointer to datetime variable 1
* @param dt2 [in] pointer to datetime variable 2
* @return true if both datatime variables are the same or false if not
*******************************************************************************/
bool datetime_is_equal(const datetime_t * dt1, const datetime_t * dt2)
{
    return (memcmp(dt1, dt2, sizeof(datetime_t)) == 0);
}

/***************************************************************************//**
* @brief Convert a time variable to seconds
* @param time [in] time variable to compare to seconds
* @return time variable converted to seconds
*******************************************************************************/
int datetime_time_to_sec(const datetime_t * time)
{
    int sec = (int) time->sec;
    sec += ((int) time->min) * 60;
    sec += ((int) time->hour) * 3600;
    return sec;
}

/***************************************************************************//**
* @brief Get difference in seconds between time1 and time2
* @param time1 [in] time variable 1
* @param time2 [in] time variable 2
* @return difference in seconds: time1 - time2
*******************************************************************************/
int datetime_time_diff(const datetime_t * time1, const datetime_t * time2)
{
    return (datetime_time_to_sec(time1) - datetime_time_to_sec(time2));
}
