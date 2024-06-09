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
 * uart_drv - the UART communication driver (implemented in non-blocking mode,
 * not multi-thread safe)
 ******************************************************************************/
#ifndef UART_DRV_H
#define UART_DRV_H

//******************************************************************************
// Includes
//******************************************************************************

//******************************************************************************
// Defines
//******************************************************************************
#define UART_TX_PIN     0
#define UART_RX_PIN     1

#define UART_ID         uart0
#define UART_IRQ_PRIO   PICO_LOWEST_IRQ_PRIORITY
#define UART_BR         115200
#define UART_TX_BUFF    65536
#define UART_RX_BUFF    128

#define UART_DBG_LVL    1

//******************************************************************************
// Exported Functions
//******************************************************************************

// UART Init function
void uart_drv_init();

// Write text to UART
void uart_drv_puts(const char * txt);

// Write formatted data from variable argument list to UART
//int uart_drv_printf0(const char* format, ...);

// Write formatted data from variable argument list to UART (using UART_DBG_LVL)
//int uart_drv_printf(const char* format, ...);

// Send to UART a buffer of data in a hexadecimal 'dump' format
//void uart_drv_dump(const void * ptr_buffer, int len, unsigned long addr);

// Check if UART interrupt is active
bool uart_drv_checkIrq(void);

// Check if there are received characters and copy them to buffer
int uart_drv_getRx(char * buff, int buff_max_len);

//******************************************************************************
#endif /* UART_DRV_H */
