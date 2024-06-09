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

//******************************************************************************
// Includes
//******************************************************************************
#include <stdint.h>
#include <stdio.h>

#include "pico/stdlib.h"

#include "gpio_drv.h"
#include "test_btn.h"

//******************************************************************************
// Function Prototypes
//******************************************************************************

//******************************************************************************
// Global Variables
//******************************************************************************

static InputFilter infBtn1;
static InputFilter infBtn2;
static InputFilter encChA;
static InputFilter encChB;
static Encoder enc1;

static int bcnt = 0;

/***************************************************************************//**
* @brief Init test_btn module. Must be called in init-phase.
*******************************************************************************/
void test_btn_init(void)
{
    input_filter_init(&infBtn1);
    input_filter_init(&infBtn2);
    input_filter_init(&encChA);
    input_filter_init(&encChB);
    encoder_init(&enc1);
}

/***************************************************************************//**
* @brief Test btn polling function. Must be called every program-cycle.
*******************************************************************************/
void test_btn_poll(void)
{
    // Button 1
    input_filter(&infBtn1, !gpio_get(GPIO_P_BTN1));
    gpio_put(GPIO_P_LED0, infBtn1.state ? 1 : 0);
    if(infBtn1.state != infBtn1.state_old)
    {
        if(infBtn1.state)
        {
            TEST_BTN_PRINTF("The button is %s\r\n", "pressed");
            TEST_BTN_PRINTF("The button pressed %i times %i \r\n", ++bcnt, enc1.pos);
            encoder_init(&enc1);
        }
        else{
            TEST_BTN_PRINTF("The button is %s\r\n", "released");
        }
        
        infBtn1.state_old = infBtn1.state;
    }

    // Button 2
    input_filter(&infBtn2, !gpio_get(GPIO_P_BTN2));
    if(infBtn2.state != infBtn2.state_old)
    {
        if(infBtn2.state)
        {
            uint32_t * ptr = (uint32_t *)(0x4001c000ul + (uint32_t )enc1.pos);
            TEST_BTN_PRINTF("Addr[%i] %#x: %#x\r\n", enc1.pos, (uint32_t)ptr, *ptr); 
        }
        infBtn2.state_old = infBtn2.state;
    }

    // Encoder
    input_filter(&encChA, gpio_get(GPIO_P_ENCA));
    input_filter(&encChB, gpio_get(GPIO_P_ENCB));
    encoder_poll(&enc1, encChA.state, encChB.state);
    if((enc1.pos_old != enc1.pos)
    || (enc1.dir_old != enc1.dir))
    {
        TEST_BTN_PRINTF("Info: flags=%02X, quad=%i, pos=%i, dir=%i, %s\r\n", 
            enc1.flags, enc1.quad, enc1.pos, enc1.dir, (enc1.dir_old != enc1.dir) ? " <---" : "");

        enc1.pos_old = enc1.pos;
        enc1.dir_old = enc1.dir;
    }
}
