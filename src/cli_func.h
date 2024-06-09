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
 * cli_func - Command Line Interface Functions
 * Defines all functions available in CLI
 ******************************************************************************/
#ifndef CLI_FUNC_H
#define CLI_FUNC_H

//******************************************************************************
// Includes
//******************************************************************************

//******************************************************************************
// Defines
//******************************************************************************

//******************************************************************************
// Typedefs
//******************************************************************************

//******************************************************************************
// Exported Variables
//******************************************************************************
extern int cli_test_val1;
extern int cli_test_val2;
extern int cli_display;
extern int cli_intens;

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init CLI functions
void cli_func_init(void);

//******************************************************************************
#endif /* CLI_FUNC.H */
