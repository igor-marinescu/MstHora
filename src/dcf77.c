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
 * dcf77 - the module scans and decodes the DCF77 signal from DCF77-Radio Board.
 ******************************************************************************/

//******************************************************************************
// Includes
//******************************************************************************
#include <stdint.h>
#include <string.h> // memcpy
#include <stdio.h>
#include <limits.h> // INT_MAX

#include "pico/stdlib.h"

#include "dcf77.h"
#include "gpio_drv.h"
#include "utils.h"

//******************************************************************************
// Function Prototypes
//******************************************************************************
void dcf_sig_quality(const ustime_t sys_ustime);

//******************************************************************************
// Global Variables
//******************************************************************************
static bool pin_old = false;    // Previous state of input signal

static dcf_bit_t pulse;     // Currently analized pulse
static dcf_bit_t bit_s;     // Last detected valid bit
static dcf_bit_t bit_c;     // Copy of the last detected valid bit

static bool sync_detected = false;  // Set when a new sync condiftion detected
static bool sync_valid = false;     // Set indicates the sync is valid
static ustime_t sync_time;          // Time in us when last sync was detected

static int8_t rx_bits_val[60];  // Received bits values
static int    rx_bits_len[60];  // Received bits length

// Variables used to store valid time/date
static datetime_t dt_last;              // Last detected datetime
static bool dt_last_valid = false;      // Flag indicates last detected datetime is valid
static int cnt_dt_valid = 0;            // Counter for consecutive valid datatimes

// Variables used to measure signal quality
static int q_good_cnt = 0;  // Count of good detected pulses
static int q_bad_cnt = 0;   // Count of bad pulses
static ustime_t q_time;     // Time in us for statistics period
static int q_filter_buff[DCF_Q_FILTER];
static m_filter_int_t q_filter;
static int q_quality = 0;   // Quality value in %, 0%-bad signal, 100%-good signal

/***************************************************************************//**
* @brief Init DCF77 Module. Must be called in main in init phase
*******************************************************************************/
void dcf_init(void)
{
    gpio_init(DCF_IN_PIN);
    gpio_set_dir(DCF_IN_PIN, GPIO_IN);
    gpio_set_pulls(DCF_IN_PIN, /*pull-up*/ false, /*pull-down*/ false);

    DCF_CLEAR_BIT(pulse);
    DCF_CLEAR_BIT(bit_s);
    DCF_CLEAR_BIT(bit_c);

    m_filter_int_init(&q_filter, q_filter_buff, DCF_Q_FILTER);
}

/***************************************************************************//**
* @brief Analyze input signal. Check if the pulse is a valid bit (0 or 1) or 
*        if a sync condition occurs.
*        The function must be called when the input signal state has changed.
* @param sys_ustime [in] System time in us
* @param pin_val [in] the input signal state
*******************************************************************************/
static void analyze_pulse(const ustime_t sys_ustime, bool pin_val)
{
    if(pin_val)
    {
        // Pulse start ___|---

        // Detect sync (if the falling edge was detected)
        if((pulse.edge & DCF_EDGE_FALLING) != 0)
        {
            ustime_t len = get_diff_ustime(sys_ustime, pulse.end);
            if(len > DCF_SYNC_MIN)
            {
                sync_detected = true;
                sync_time = sys_ustime;
                DCF_LOG("sync\r\n");
                //TP_TGL(LOG_CH6);
            }
        }

        DCF_CLEAR_BIT(pulse);
        pulse.edge = DCF_EDGE_RAISING;
        pulse.start = sys_ustime;
    }
    else {
        // Pulse end ---|___
        pulse.edge |= DCF_EDGE_FALLING;
        pulse.end = sys_ustime;
        ustime_t len = get_diff_ustime(pulse.end, pulse.start);
        pulse.len = len;

        // Both edges detected? ___|---|___
        if(pulse.edge == (DCF_EDGE_RAISING | DCF_EDGE_FALLING))
        {
            if((len > DCF_BIT0_MIN) && (len < DCF_BIT0_MAX))
            {
                pulse.val = dcf_bitval_false;
            }
            else if((len > DCF_BIT1_MIN) && (len < DCF_BIT1_MAX))
            {
                pulse.val = dcf_bitval_true;
            }
            else if(len > DCF_BIT1_MAX)
            {
                sync_valid = false;   
            }

            // Valid bit detected, copy it
            if(pulse.val != dcf_bitval_none)
            {
                DCF_COPY_BIT(bit_s, pulse);
                DCF_COPY_BIT(bit_c, pulse);
                q_good_cnt++;
            }
            else {
                // Not a valid bit, restore last valid detected bit as pulse
                // this will help to detect new sync
                DCF_COPY_BIT(pulse, bit_c);
                q_bad_cnt++;
            }
        }
    }
}

/***************************************************************************//**
* @brief Store bit_s (if valid) in the buffer of received bits
*******************************************************************************/
static void store_bit_s(void)
{
    ustime_t del_time;
    int32_t bit_idx;
    int32_t rest;

    // Calculate bit index based on delay between sync and detected bit
    del_time = get_diff_ustime(bit_s.start, sync_time);
    
    bit_idx = (int32_t) del_time / DCF_T_1SEC;
    rest = ((int32_t) del_time) - (bit_idx * DCF_T_1SEC);
    if(rest > 500000L)
        bit_idx++;

    DCF_LOG("%5i | %2i | %3i | %i \r\n", del_time / 1000L, bit_idx, bit_s.len / 1000L, (bit_s.val < 0) ? 0 : 1);

    if((bit_idx >= 0L) && (bit_idx < sizeof(rx_bits_val)))
    {
        // Bit already has a value? Keep the old value only if the new bit is shorter
        if((rx_bits_val[bit_idx] != 0) && ((rx_bits_len[bit_idx] > bit_s.len)))
            return;

        rx_bits_val[bit_idx] = bit_s.val;
        rx_bits_len[bit_idx] = bit_s.len;
    }
}

/***************************************************************************//**
* @brief Extract a cnt number of bits from received bits buffer (rx_bits) 
*        starting from position start_idx and convert them to uint8 value  
* @param ptr_out [out] pointer to uint8 where the extracted value is stored
* @param start_idx [in] index of the start position to extract bits
* @param end_idx [in] index of the end position to extract bits
* @return true - uint8 successfully extracted or false in case the value 
*         cannot be extracted (there are undefined bits)
*******************************************************************************/
static bool rx_bits_to_uint8(uint8_t * ptr_out, int start_idx, int end_idx)
{
    if((start_idx < 0) || (end_idx >= sizeof(rx_bits_val)))
        return false;

    uint8_t val = 0;
    uint8_t mask = 1;
    while(start_idx <= end_idx)
    {
        if(rx_bits_val[start_idx] == dcf_bitval_none)
            return false;
        if(rx_bits_val[start_idx] == dcf_bitval_true)
            val |= mask;

        mask <<= 1;
        start_idx++;
    }
    *ptr_out = val;
    return true;
}

/***************************************************************************//**
* @brief Calculate the parity sum of a byte. The parity is calculated by counting
*        the ones in a byte. Example: 0x49 -> parity=3, 0x50 -> parity=2
* @param par_val [in] initial value of the parity sum (used when the parity over more
*        than one byte must be calculated, otherwise must be 0)
* @param val [in] input byte which parity sum must be calculated
* @return return the parity sum = new_parity + par_val
*******************************************************************************/
static int calc_parity(int par_sum, uint8_t val)
{
    while(val > 0)
    {
        if((val & 1) != 0)
            par_sum++;
        val >>= 1;
    }
    return par_sum;
}

/***************************************************************************//**
* @brief Check if a parity sum (calculated using calc_parity) satisfies a given
*        parity (defined as dcf_bitval_t)
* @param par_val [in] parity sum, calculated using calc_parity
* @param parity [in] parity (defined as dcf_bitval_t)
* @return true - the parity sum satisfies the parity
*         false - the parity sum doesn't satisfy the parity, 
*                 or the parity is not defined
*******************************************************************************/
static bool check_parity(int par_sum, dcf_bitval_t parity)
{
    if((parity == dcf_bitval_true) && (par_sum & 1))
        return true;
    if((parity == dcf_bitval_false) && !(par_sum & 1))
        return true;

    return false;
}

/***************************************************************************//**
* @brief Extract time from received DCF77 telegram
* @param ptr_datetime [out] pointer to datetime variable where extracted time is stored
*        in case it contains a valid value
* @return true - time successfully extracted and copied to ptr_datetime
*         false - could not extract time, received DCF77 telegram has invalid time
*******************************************************************************/
static bool rx_bits_extract_time(datetime_t * ptr_datetime)
{
    uint8_t val;
    datetime_t dt;
    dt.sec = 0;

    // Extract Minutes
    if(rx_bits_val[28] == dcf_bitval_none)
    {
        DCF_LOG("Error: min parity[28] undefined\r\n");
        return false;
    }
    if(!rx_bits_to_uint8(&val, 21, 27))
    {
        DCF_LOG("Error: min[21..27] undefined\r\n");
        return false;
    }
    if(!check_parity(calc_parity(0, val), rx_bits_val[28]))
    {
        DCF_LOG("Error: min parity value: %i\r\n", (int) rx_bits_val[28]);
        return false;
    }
    dt.min = bcd_to_int8(val);

    // Extract hour
    if(rx_bits_val[35] == dcf_bitval_none)
    {
        DCF_LOG("Error: hr parity[35] undefined\r\n");
        return false;
    }
    if(!rx_bits_to_uint8(&val, 29, 34))
    {
        DCF_LOG("Error: hr[29..34] undefined\r\n");
        return false;
    }
    if(!check_parity(calc_parity(0, val), rx_bits_val[35]))
    {
        DCF_LOG("Error: hr parity value: %i\r\n", (int) rx_bits_val[35]);
        return false;
    }
    dt.hour = bcd_to_int8(val);

    if(!datetime_is_valid_time(&dt))
    {
        //DCF_LOG("Error: time invalid: %i:%02i\r\n", dt.hour, dt.min);
        DCF_LOG_TIME("Error: time invalid:", dt, "\r\n");
        return false;
    }

    datetime_copy_time(ptr_datetime, &dt);
    //DCF_LOG("%i:%02i\r\n", dt.hour, dt.min);
    DCF_LOG_TIME("", dt, "\r\n");
    return true;
}

/***************************************************************************//**
* @brief Extract date from received DCF77 telegram
* @param ptr_datetime [out] pointer to datetime variable where extracted date is stored
*        in case it contains a valid value
* @return true - date successfully extracted and copied to ptr_datetime
*         false - could not extract date, received DCF77 telegram has invalid date
*******************************************************************************/
static bool rx_bits_extract_date(datetime_t * ptr_datetime)
{
    int parity = 0;
    uint8_t val;
    datetime_t dt;

    if(rx_bits_val[58] == dcf_bitval_none)
    {
        DCF_LOG("Error: date parity[58] undefined\r\n");
        return false;
    }

    // Day of month
    if(!rx_bits_to_uint8(&val, 36, 41))
    {
        DCF_LOG("Error: dd[36..41] undefined\r\n");
        return false;
    }
    dt.day = bcd_to_int8(val);
    parity = calc_parity(parity, val);

    // Day of week
    if(!rx_bits_to_uint8(&val, 42, 44))
    {
        DCF_LOG("Error: day[42..22] undefined\r\n");
        return false;
    }
    dt.dotw = bcd_to_int8(val);
    // Convert RTC (Mon=1... Sun=7) to datetime_t (Sun=0, Mon=1..Sat=6)
    if(dt.dotw == 7)
        dt.dotw = 0;
    parity = calc_parity(parity, val);

    // Month
    if(!rx_bits_to_uint8(&val, 45, 49))
    {
        DCF_LOG("Error: month[45..49] undefined\r\n");
        return false;
    }
    dt.month = bcd_to_int8(val);
    parity = calc_parity(parity, val);

    // Year
    if(!rx_bits_to_uint8(&val, 50, 57))
    {
        DCF_LOG("Error: year[50..57] undefined\r\n");
        return false;
    }
    dt.year = (int16_t) bcd_to_int8(val);
    dt.year += 2000;
    parity = calc_parity(parity, val);

    if(!check_parity(parity, rx_bits_val[58]))
    {
        DCF_LOG("Error: date parity value: %i\r\n", (int) rx_bits_val[58]);
        return false;
    }
    
    if(!datetime_is_valid_date(&dt))
    {
        //DCF_LOG("Error: date invalid: %i.%i.%i (%i)\r\n", dt.day, dt.month, dt.year, dt.dotw);
        DCF_LOG_DATE("Error: date invalid: ", dt, "\r\n");
        return false;
    }

    datetime_copy_date(ptr_datetime, &dt);
    //DCF_LOG("%i.%i.%i (%i)\r\n", dt.day, dt.month, dt.year, dt.dotw);
    DCF_LOG_DATE("", dt, "\r\n");
    return true;
}

/***************************************************************************//**
* @brief Interpret received data bits. Try to decode the information.
*        Profiling: the function takes 96..216us with logs, 10..100us without logs.
* @param sys_ustime [in] system time in us
* @return true if dcf telegram successfully decoded and both time and date are
*         considered valid.
*******************************************************************************/
static bool interpret_rx_bits(const ustime_t sys_ustime)
{
    DCF_LOG("interpret_rx_bits\r\n");

    if(rx_bits_val[0] != dcf_bitval_false)
    {
        DCF_LOG("Error: bit[0] != 0\r\n");
        return false;
    }

    if(rx_bits_val[20] != dcf_bitval_true)
    {
        DCF_LOG("Error: bit[20] != 1\r\n");
        return false;
    }

    datetime_t dt;
    bool valid_time = rx_bits_extract_time(&dt);
    if(valid_time)
    {
        // If the last datetime valid, check if the difference 
        // between new and last time is ~1 minute (30 sec ... 90 sec)
        if(dt_last_valid)
        {
            int diff = datetime_time_diff(&dt, &dt_last);
            DCF_LOG("diff = %i\r\n", diff);

            if((diff < 30) || (diff > 90))
            {
                // Something went wrong, too big difference
                // probably we lost a sync signal
                cnt_dt_valid = 1;
            }
        }
    }

    bool valid_date = rx_bits_extract_date(&dt);
    if(valid_date)
    {
        // If the last date is valid, check if it is the same
        // like the new date. If yes increment validity counter
        if(dt_last_valid)
        {
            if(datetime_date_compare(&dt, &dt_last) != 0)
            {
                // Something went wrong, there is a date difference
                // probably day change
                cnt_dt_valid = 1;
            }
        }
    }

    dt_last_valid = (valid_time && valid_date);
    if(dt_last_valid)
    {
        datetime_copy(&dt_last, &dt);
        if(cnt_dt_valid < INT_MAX)
            cnt_dt_valid++;
        if(cnt_dt_valid >= DCF_VALID_DATETIME_CNT)
        {
            DCF_LOG("confirmed valid datetime (cnt=%i)\r\n", cnt_dt_valid);
        }
    }
    else {
        cnt_dt_valid = 0;
    }

    return (dt_last_valid && (cnt_dt_valid >= DCF_VALID_DATETIME_CNT));
}

/***************************************************************************//**
* @brief DCF77 polling function. Must be called every program cycle
* @param sys_ustime [in] System time in us
* @return true if dcf telegram successfully decoded and both time and date are
*         considered valid.
*******************************************************************************/
bool dcf_poll(const ustime_t sys_ustime)
{
    bool pin_val = !gpio_get(DCF_IN_PIN);

    // Input signal state changed? Analyze it
    if(pin_val != pin_old)
    {
        analyze_pulse(sys_ustime, pin_val);

        pin_old = pin_val;
        // Check the results in another cycle
        // to split the calculation time
        return false;
    }

    // Measuring signal quality
    dcf_sig_quality(sys_ustime);

    // Sync detected?
    if(sync_detected)
    {
        sync_detected = false;
        sync_valid = true;
        DCF_CLEAR_BIT(bit_s);
        memset(rx_bits_val, 0, sizeof(rx_bits_val));
        memset(rx_bits_len, 0, sizeof(rx_bits_len));
        return false;
    }

    // Do not check pulse if there is no valid sync
    if(!sync_valid)
        return false;

    // Check if sync timeout
    if(get_diff_ustime(sys_ustime, sync_time) > DCF_T_59SEC)
    {
        sync_valid = false;
        DCF_LOG("sync timeout\r\n");
        return interpret_rx_bits(sys_ustime); 
    }

    // New valid bit detected? Store it
    if(bit_s.val != dcf_bitval_none)
    {
        store_bit_s();
        DCF_CLEAR_BIT(bit_s);
    }
    return false;
}

/***************************************************************************//**
* @brief Measuring signal quality
*        Profiling: the function takes 36us with logs, 1.5us without logs.
* @param sys_ustime [in] System time in us
*******************************************************************************/
void dcf_sig_quality(const ustime_t sys_ustime)
{
    if(get_diff_ustime(sys_ustime, q_time) > DCF_T_1SEC)
    {
        q_time = sys_ustime;

        //DCF_LOG("g=%2i, b=%2i", q_good_cnt, q_bad_cnt);
        int quality = 0;

        // Perfect signal: 1 good pulse pro secunde, no bad pulses
        if((q_good_cnt == 1) && (q_bad_cnt == 0))
        {
            quality = 100;
        }
        else{
            // If there are > 1 good pulses pro secunde,
            // the difference is the amount of bad pulses
            // Add them to the bad pulses
            if(q_good_cnt > 1)
                q_bad_cnt += (q_good_cnt - 1);

            // Based on count of bad pulses we analyse signal quality:
            // more bad pulses -> worst signal
            if(q_bad_cnt > 0)
            {
                if(q_bad_cnt >= 5)
                    quality = 0;
                else
                    quality = (5 - q_bad_cnt) * 20;
            }
            else {
                // In case there are no pulse detected
                // the signal is missing
                quality = 0;
            }
        }

        // Apply a median filter on quality variable
        q_quality = m_filter_int_add_val(&q_filter, quality);

        //DCF_LOG(" | g=%2i, b=%2i, q=%3i | %i \r\n", q_good_cnt, q_bad_cnt, quality, q_quality);
        q_good_cnt = 0;
        q_bad_cnt = 0;
    }
}

/***************************************************************************//**
* @brief Get signal quality 0..100%
* @return signal quality 0..100%
*******************************************************************************/
int dcf_get_quality(void)
{
    return q_quality;
}

/***************************************************************************//**
* @brief Return last detected datetime if valid
* @param datetime_ptr [out] pointer to destination datetime where the datetime is copied
* @return pointer of the last detected valid datetime or NULL in case
*         if there is no valid datetime detected
*******************************************************************************/
datetime_t *  dcf_get_datetime(void)
{
    if(dt_last_valid)
    {
        return &dt_last;
    }
    return NULL;
}
