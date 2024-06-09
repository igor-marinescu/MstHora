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
#ifndef GPIO_DRV_H
#define GPIO_DRV_H

//******************************************************************************
// Includes
//******************************************************************************

//******************************************************************************
// Defines
//******************************************************************************
#define TP_SET(x)   gpio_put((x), 1)
#define TP_CLR(x)   gpio_put((x), 0)
#define TP_TGL(x)   gpio_put((x), !gpio_get(x))

//          UART_TX 0 
//          UART_RX 1
//              GND
#define GPIO_P_BTN1 2
#define GPIO_P_BTN2 3
#define GPIO_P_LED0 4
#define GPIO_P_LED1 5
//              GND
#define GPIO_P_LED2 6
#define GPIO_P_ENSW 7
#define GPIO_P_ENCA 8
#define GPIO_P_ENCB 9
//              GND
//                  10
//                  11
//                  12
//                  13
//              GND

#define LOG_CH2     26
#define LOG_CH3     27
#define LOG_CH4     20
#define LOG_CH5     21
#define LOG_CH6     22

// GP0 ->|1        40|<- VBUS
// GP1 ->|2        39|<- VSYS
// GND ->|3        38|<- GND
// GP2 ->|4        37|<- 3V3_EN
// GP3 ->|5        36|<- 3V3(OUT)
// GP4 ->|6        35|<- ADC_VREF
// GP5 ->|7        34|<- GP28
// GND ->|8        33|<- GND
// GP6 ->|9        32|<- GP27
// GP7 ->|10       31|<- GP26
// GP8 ->|11       30|<- RUN
// GP9 ->|12       29|<- GP22
// GND ->|13       28|<- GND
// GP10->|14       27|<- GP21
// GP11->|15       26|<- GP20
// GP12->|16       25|<- GP19
// GP13->|17       24|<- GP18
// GND ->|18       23|<- GND
// GP14->|19       22|<- GP17
// GP15->|20       21|<- GP16

// Filter defines
#define InputFilterMax  2000

typedef struct {
    int cnt;
    bool state;
    bool state_old;
} InputFilter;

// Encoder defines
#define ENC_CHA     0x01
#define ENC_CHB     0x02
#define ENC_CHA0    0x04
#define ENC_CHB0    0x08

#define ENC_DIR_P   0
#define ENC_DIR_N   1

#define SAVE_QUAD_BITS(x)   x &= ~(ENC_CHA0 | ENC_CHB0); \
                            x |= ((x << 2) & (ENC_CHA0 | ENC_CHB0))

typedef struct {
    uint8_t flags;
    int quad;
    int pos;
    int pos_old;
    int dir;
    int dir_old;
} Encoder;

//******************************************************************************
// Exported Functions
//******************************************************************************

// Init GPIO pins
void gpio_drv_init();

// Init input filter
void input_filter_init(InputFilter * ptr_in);

// Filter input signal
void input_filter(InputFilter * ptr_in, bool signal_in);

// Init encoder
void encoder_init(Encoder * ptr_enc);

// Poll encoder/decode quadrature CH-A/CH-B signals
void encoder_poll(Encoder * ptr_enc, bool ch_a, bool ch_b);

//******************************************************************************
#endif /* GPIO_DRV_H */
