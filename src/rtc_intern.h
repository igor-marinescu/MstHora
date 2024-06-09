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
 * rtc_intern - the driver for the internal (integrated) RTC module.
 ******************************************************************************/
#ifndef RTC_INTERN_H
#define RTC_INTERN_H

//******************************************************************************
// Includes
//******************************************************************************
#include "datetime_utils.h"
#include "ustime.h"

#ifdef RTC_INTERN_DEBUG
#include DEBUG_INCLUDE
#endif

//******************************************************************************
// Defines
//******************************************************************************
#ifdef RTC_INTERN_DEBUG
#define RTC_INT_LOG(...)     DEBUG_PRINTF(__VA_ARGS__)
#else
#define RTC_INT_LOG(...)    
#endif

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init RTC intern module
void rtc_int_init(void);

// RTC intern module polling function. Must be called every program cycle
bool rtc_int_poll(const ustime_t sys_ustime);

// Set time and date to RTC intern module
bool rtc_int_set(datetime_t * ptr_datetime);

// Return RTC intern actual datetime
datetime_t * rtc_int_get_datetime(void);

//******************************************************************************
#endif /* RTC_INTERN_H */
