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
 * spi_drv - the SPI communication driver (implemented in non-blocking mode,
 * not multi-thread safe)
 ******************************************************************************/
#ifndef SPI_DRV_H
#define SPI_DRV_H

//******************************************************************************
// Includes
//******************************************************************************

//******************************************************************************
// Defines
//******************************************************************************
#define SPI_DRV_ID          spi0
#define SPI_DRV_IRQ         SPI0_IRQ
#define SPI_DRV_BAUDRATE    100000

#define SPI_DRV_TX_PIN      19
#define SPI_DRV_RX_PIN      16
#define SPI_DRV_SCK_PIN     18
#define SPI_DRV_CS_PIN      17

#define SPI_DRV_BUF_LEN     0x10
#define SPI_DRV_DUMMY_BYTE  0x00

#define SPI_DRV_CS_SET()    gpio_put(SPI_DRV_CS_PIN, 0)
#define SPI_DRV_CS_CLR()    gpio_put(SPI_DRV_CS_PIN, 1)

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init SPI driver
void spi_drv_init(void);

// Initiate SPI data transfer (non blocking function)
bool spi_drv_send(const uint8_t * tx_buff, int len);

// Return busy-status of the SPI
bool spi_drv_is_busy(void);

//******************************************************************************
#endif /* SPI_DRV_H */
