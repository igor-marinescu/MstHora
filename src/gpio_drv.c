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
 * gpio_drv - the GPIO module functions and definitions.
 ******************************************************************************/

//******************************************************************************
// Includes
//******************************************************************************
#include <string.h>
#include "pico/stdlib.h"
#include "gpio_drv.h"

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// Global Variables
//******************************************************************************

/***************************************************************************//**
* @brief Init GPIO pins
*******************************************************************************/
void gpio_drv_init()
{
    gpio_init(GPIO_P_LED0);
    gpio_set_dir(GPIO_P_LED0, GPIO_OUT);

    gpio_init(GPIO_P_BTN1);
    gpio_set_dir(GPIO_P_BTN1, GPIO_IN);
    gpio_set_pulls(GPIO_P_BTN1, /*pull-up*/ false, /*pull-down*/ false);

    gpio_init(GPIO_P_BTN2);
    gpio_set_dir(GPIO_P_BTN2, GPIO_IN);
    gpio_set_pulls(GPIO_P_BTN2, /*pull-up*/ false, /*pull-down*/ false);

    gpio_init(GPIO_P_ENCA);
    gpio_set_dir(GPIO_P_ENCA, GPIO_IN);
    gpio_set_pulls(GPIO_P_ENCA, /*pull-up*/ false, /*pull-down*/ false);

    gpio_init(GPIO_P_ENCB);
    gpio_set_dir(GPIO_P_ENCB, GPIO_IN);
    gpio_set_pulls(GPIO_P_ENCB, /*pull-up*/ false, /*pull-down*/ false);

    gpio_init(LOG_CH2);
    gpio_set_dir(LOG_CH2, GPIO_OUT);

    gpio_init(LOG_CH3);
    gpio_set_dir(LOG_CH3, GPIO_OUT);

    gpio_init(LOG_CH4);
    gpio_set_dir(LOG_CH4, GPIO_OUT);

    gpio_init(LOG_CH5);
    gpio_set_dir(LOG_CH5, GPIO_OUT);

    gpio_init(LOG_CH6);
    gpio_set_dir(LOG_CH6, GPIO_OUT);
}

/***************************************************************************//**
* @brief Init input filter
* @param ptr_in [in/out] pointer to filter structure to init.
*******************************************************************************/
void input_filter_init(InputFilter * ptr_in)
{
    memset(ptr_in, 0, sizeof(InputFilter));
}

/***************************************************************************//**
* @brief Filter input signal
* @param ptr_in [in/out] pointer to filter structure
* @param signal_in [in] input signal to filter
*******************************************************************************/
void input_filter(InputFilter * ptr_in, bool signal_in)
{
    if(signal_in)
    {
        if(ptr_in->cnt < InputFilterMax)
            ptr_in->cnt++;
        else
            ptr_in->state = true;
    }
    else {
        if(ptr_in->cnt > 0)
            ptr_in->cnt--;
        else
            ptr_in->state = false;                
    }
}

/***************************************************************************//**
* @brief Init encoder
* @param ptr_enc [in/out] pointer to encoder structure to init.
*******************************************************************************/
void encoder_init(Encoder * ptr_enc)
{
    memset(ptr_enc, 0, sizeof(Encoder));
}

/***************************************************************************//**
* @brief Poll encoder/decode quadrature CH-A/CH-B signals
* @param ptr_enc [in/out] pointer to encoder structure
* @param ch_a [in] CH-A input signal
* @param ch_b [in] CH-B input signal
*******************************************************************************/
void encoder_poll(Encoder * ptr_enc, bool ch_a, bool ch_b)
{
    // Set/Reset Encoder1 input pins
    ptr_enc->flags &= ~(ENC_CHB | ENC_CHA);
    if(ch_a)
        ptr_enc->flags |= ENC_CHA;
    if(ch_b)
        ptr_enc->flags |= ENC_CHB;

    // Encoder 1 Position detect
    switch(ptr_enc->flags & 0x0F){
    //-------------------- BA BA ------------------------------------
    // No change          OLD NEW
    case 0x00:          // 00 00
    case 0x05:          // 01 01
    case 0x0A:          // 10 10
    case 0x0F:          // 11 11
        break;
        
    // Error
    case 0x03:          // 00 11
    case 0x06:          // 01 10
    case 0x09:          // 10 01
    case 0x0C:          // 11 00
        SAVE_QUAD_BITS(ptr_enc->flags);
        break;

    // Forward
    case 0x01:          // 00 01: 1->2
    case 0x07:          // 01 11: 2->3
    case 0x0E:          // 11 10: 3->4
    case 0x08:          // 10 00: 4->1
        ptr_enc->quad++;
        ptr_enc->pos = ptr_enc->quad >> 1;
        SAVE_QUAD_BITS(ptr_enc->flags);
        ptr_enc->dir = ENC_DIR_P;
        break;

    // Backward
    case 0x0B:          // 10 11: 4->3
    case 0x0D:          // 11 01: 3->2
    case 0x04:          // 01 00: 2->1
    case 0x02:          // 00 10: 1->4
        ptr_enc->quad--;
        ptr_enc->pos = ptr_enc->quad >> 1;
        SAVE_QUAD_BITS(ptr_enc->flags);
        ptr_enc->dir = ENC_DIR_N;
        break;
    }
}
