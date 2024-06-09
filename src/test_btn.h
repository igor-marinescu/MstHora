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
 * test_btn - the module to test the buttons and encoders functionality.
 ******************************************************************************/
#ifndef TEST_BTN_H
#define TEST_BTN_H

//******************************************************************************
// Includes
//******************************************************************************
#include DEBUG_INCLUDE

//******************************************************************************
// Defines
//******************************************************************************
#define TEST_BTN_PRINTF(...)            DEBUG_PRINTF(__VA_ARGS__)
#define TEST_BTN_DUMP(buff, len, addr)  DEBUG_DUMP((buff), (len), (addr))

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init test_btn module
void test_btn_init(void);

// Test button polling function
void test_btn_poll(void);

//******************************************************************************
#endif /* TEST_BTN_H */
