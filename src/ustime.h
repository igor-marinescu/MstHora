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
 * ustime - calculates the difference between two ustime_t/s_time_t data.
 ******************************************************************************/
#ifndef USTIME_H
#define USTIME_H

//******************************************************************************
// Includes
//******************************************************************************

//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Typedefs
//******************************************************************************

// System time in micro-seconds
typedef uint32_t ustime_t;

// System time in seconds
typedef uint32_t s_time_t;

//******************************************************************************
// Exported Functions
//******************************************************************************

// Get the difference (always positive) between two ustime_t variables
ustime_t get_diff_ustime(const ustime_t end_time, const ustime_t start_time);

// Get the difference (always positive) between two s_time_t variables
s_time_t get_diff_s_time(const s_time_t end_time, const s_time_t start_time);

//******************************************************************************
#endif /* USTIME_H */
