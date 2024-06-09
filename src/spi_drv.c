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

//******************************************************************************
// Includes
//******************************************************************************
#include <string.h>
#include <stdio.h>
#include <stdarg.h>     //varg

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/irq.h"
#include "spi_drv.h"
#include "gpio_drv.h"

//******************************************************************************
// Function Prototypes
//******************************************************************************
void spi_drv_irq();

//******************************************************************************
// Global Variables
//******************************************************************************
uint8_t spi_drv_rx_buf[SPI_DRV_BUF_LEN];
uint8_t spi_drv_tx_buf[SPI_DRV_BUF_LEN];

volatile int spi_drv_rx_len = 0;
volatile int spi_drv_tx_len = 0;
volatile int spi_drv_tx_idx = 0;
volatile bool spi_drv_busy_flag = false;

/***************************************************************************//**
* @brief Init SPI driver
*******************************************************************************/
void spi_drv_init(void)
{
    // Enable SPI 0 at 1 MHz and connect to GPIOs
    spi_init(SPI_DRV_ID, SPI_DRV_BAUDRATE);
    gpio_set_function(SPI_DRV_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_DRV_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_DRV_TX_PIN, GPIO_FUNC_SPI);
    //gpio_set_function(SPI_DRV_CS_PIN, GPIO_FUNC_SPI);

    // CS manually trigered
    gpio_init(SPI_DRV_CS_PIN);
    gpio_set_dir(SPI_DRV_CS_PIN, GPIO_OUT);
    SPI_DRV_CS_CLR();

    // Set up and enable the interrupt handlers
    irq_set_exclusive_handler(SPI_DRV_IRQ, spi_drv_irq);
    irq_set_enabled(SPI_DRV_IRQ, true);

    // Disable all SPI interrupts
    spi_get_hw(SPI_DRV_ID)->imsc = 0;
}

/***************************************************************************//**
* @brief SPI driver interrupt
*******************************************************************************/
void spi_drv_irq()
{
    uint8_t rx_data;

    // Something to send?
    if(spi_drv_tx_idx < spi_drv_tx_len)
    {
        if(spi_is_writable(SPI_DRV_ID))
        {
            if(spi_drv_tx_idx < SPI_DRV_BUF_LEN)
                spi_get_hw(SPI_DRV_ID)->dr = (uint32_t) spi_drv_tx_buf[spi_drv_tx_idx];
            else
                spi_get_hw(SPI_DRV_ID)->dr = (uint32_t) SPI_DRV_DUMMY_BYTE;
            spi_drv_tx_idx++;
        }
    }
    else{
        // Disable Tx irq, activates Rx & Tout irq only
        //hw_set_bits(spi_get_hw(SPI_DRV_ID)->imsc, SPI_SSPIMSC_TXIM_BITS)
        spi_get_hw(SPI_DRV_ID)->imsc = (SPI_SSPIMSC_RXIM_BITS | SPI_SSPIMSC_RTIM_BITS);
    }

    // Received data?
    while (spi_is_readable(SPI_DRV_ID))
    {
        rx_data = (uint8_t) spi_get_hw(SPI_DRV_ID)->dr;
        if(spi_drv_rx_len < SPI_DRV_BUF_LEN)
            spi_drv_tx_buf[spi_drv_rx_len] = rx_data;
        spi_drv_rx_len++;
    }

    // Finished sending/receiving? 
    if((spi_drv_rx_len >= spi_drv_tx_len)
    && (spi_drv_tx_idx >= spi_drv_tx_len))
    {
        // If we sent and received all data but SPI is still busy 
        // it means it is sending the last bit of the last byte
        // wait here until it finishes
        while(spi_get_hw(SPI_DRV_ID)->sr & SPI_SSPSR_BSY_BITS)
            tight_loop_contents();

        // Disable all interrupts
        spi_get_hw(SPI_DRV_ID)->imsc = 0;
        SPI_DRV_CS_CLR();
        spi_drv_busy_flag = false;
    }

    // Don't leave overrun & timeout flags set
    spi_get_hw(SPI_DRV_ID)->icr = SPI_SSPICR_RORIC_BITS | SPI_SSPICR_RTIC_BITS;
}

/***************************************************************************//**
* @brief Initiate SPI data transfer.
*        Warning! This is non-blocking function. The function doesn't wait
*        until the transfer finishes.
* @param tx_buff [in] pointer to data buffer to send
* @param len [in] count of bytes to send
* @return true if SPI has been initiated for transfer
*         false if the SPI is already busy 
*******************************************************************************/
bool spi_drv_send(const uint8_t * tx_buff, int len)
{
    if(spi_drv_busy_flag)
        return false;

    if(len > 0)
    {
        SPI_DRV_CS_SET();
        spi_drv_busy_flag = true;

        // Cleanup Rx Fifo
        while (spi_is_readable(SPI_DRV_ID))
            (void)spi_get_hw(SPI_DRV_ID)->dr;

        // Cleanup overrun & timeout flags
        spi_get_hw(SPI_DRV_ID)->icr = SPI_SSPICR_RORIC_BITS | SPI_SSPICR_RTIC_BITS;

        spi_drv_tx_len = len;
        spi_drv_tx_idx = 0;
        spi_drv_rx_len = 0;

        if(tx_buff != NULL)
        {
            if(len > SPI_DRV_BUF_LEN)
                len = SPI_DRV_BUF_LEN;
            memcpy(spi_drv_tx_buf, tx_buff, len);
        }

        // Activate Tx, Rx and Tout irqs, this initiates the transfer
        spi_get_hw(SPI_DRV_ID)->imsc = (SPI_SSPIMSC_TXIM_BITS | SPI_SSPIMSC_RXIM_BITS | SPI_SSPIMSC_RTIM_BITS);
    }
    return true;
}

/***************************************************************************//**
* @brief Return busy-status of the SPI
* @return true if the SPI is busy (transfering data)
*         false if the SPI is not busy (not transfering data)
*******************************************************************************/
bool spi_drv_is_busy(void)
{
    return spi_drv_busy_flag;
}