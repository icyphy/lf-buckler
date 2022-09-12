/*************
Copyright (c) 2022, The University of California at Berkeley.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************/

/**
 * @brief NRF52 support for the C target of Lingua Franca.
 *
 * @author{Soroush Bateni <soroush@utdallas.edu>}
 * @author{Abhi Gundrala <gundralaa@berkeley.edu>}
 * @author{Erling Jellum} <erling.r.jellum@ntnu.no>}
 * @author{Marten Lohstroh <marten@berkeley.edu>}
 */

#include <stdlib.h> // Defines malloc.
#include <string.h> // Defines memcpy.
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "lf_nrf52_support.h"
#include "../platform.h"
#include "../utils/util.h"

#include "nrf.h"
#include "nrfx_timer.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_nvic.h"
#include "nrf_soc.h"
#include "app_error.h"

/**
 * True when the last requested sleep has been completed, false otherwise.
 */
volatile bool _lf_sleep_completed = false;

/**
 * lf global timer instance
 * timerId = 3 TIMER3
 */
static const nrfx_timer_t g_lf_timer_inst = NRFX_TIMER_INSTANCE(3);

// Combine 2 32bit works to a 64 bit word
#define COMBINE_HI_LO(hi,lo) ((((uint64_t) hi) << 32) | ((uint64_t) lo))

// Maximum sleep possible
#define MAX_SLEEP_NS 1000ULL*UINT32_MAX

/**
 * Variable tracking the higher 32bits of the time
 * Incremented at each timer overflow
 */
static uint32_t _lf_time_us_high = 0;

/**
 * Flag passed to sd_nvic_critical_region_*
 */
uint8_t _lf_nested_region = 0;

/**
 * Offset to _LF_CLOCK that would convert it
 * to epoch time.
 * For CLOCK_REALTIME, this offset is always zero.
 * For CLOCK_MONOTONIC, it is the difference between those
 * clocks at the start of the execution.
 */
interval_t _lf_time_epoch_offset = 0LL;

/**
 * Convert a _lf_time_spec_t ('tp') to an instant_t representation in
 * nanoseconds.
 *
 * @return nanoseconds (long long).
 */
instant_t convert_timespec_to_ns(struct timespec tp) {
    return tp.tv_sec * 1000000000 + tp.tv_nsec;
}

/**
 * Convert an instant_t ('t') representation in nanoseconds to a
 * _lf_time_spec_t.
 *
 * @return _lf_time_spec_t representation of 't'.
 */
struct timespec convert_ns_to_timespec(instant_t t) {
    struct timespec tp;
    tp.tv_sec = t / 1000000000;
    tp.tv_nsec = (t % 1000000000);
    return tp;
}

/**
 * Calculate the necessary offset to bring _LF_CLOCK in parity with the epoch
 * time reported by CLOCK_REALTIME.
 */
void calculate_epoch_offset() {
    // unimplemented - on the nrf, probably not possible to get a bearing on real time.
    _lf_time_epoch_offset = 0LL;
}

/**
 * Handles LF timer interrupts
 * Using lf_timer instance -> id = 3
 * channel2 -> channel for lf_sleep interrupt
 * channel3 -> channel for overflow interrupt
 *
 * [in] event_type
 *      channel that fired interrupt on timer
 * [in] p_context
 *      context passed to handler
 * 
 */
void lf_timer_event_handler(nrf_timer_event_t event_type, void *p_context) {
    
    if (event_type == NRF_TIMER_EVENT_COMPARE2) {
        LF_PRINT_DEBUG("Sleep timer expired!");
        _lf_sleep_completed = true;
    } else if (event_type == NRF_TIMER_EVENT_COMPARE3) {
        _lf_time_us_high =+ 1;
        LF_PRINT_DEBUG("Overflow detected!");
    } else {
        LF_PRINT_DEBUG("Some unexpected event happened: %d", event_type);
    }
}

/**
 * Initialize the LF clock.
 */
void lf_initialize_clock() {
    _lf_time_epoch_offset = 0LL;
    _lf_time_us_high = 0;
    // initialize power management
  
    ret_code_t error_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(error_code);

    // Initialize TIMER3 as a free running timer
    // 1) Set to be a 32 bit timer
    // 2) Set to count at 1MHz
    // 3) Clear the timer
    // 4) Start the timer

    nrfx_timer_config_t timer_conf = {
        .frequency = NRF_TIMER_FREQ_1MHz,
        .mode = NRF_TIMER_MODE_TIMER,
        .bit_width = NRF_TIMER_BIT_WIDTH_32,
        .interrupt_priority = 7, // lowest
        .p_context = NULL,
    };

    error_code = nrfx_timer_init(&g_lf_timer_inst, &timer_conf, &lf_timer_event_handler);
    APP_ERROR_CHECK(error_code);
    // Enable an interrupt to occur on channel NRF_TIMER_CC_CHANNEL3
    // when the timer reaches its maximum value and is about to overflow.
    nrfx_timer_compare(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL3, 0x0, true);
    nrfx_timer_enable(&g_lf_timer_inst);
}

/**
 * Fetch the value of _LF_CLOCK (see lf_linux_support.h) and store it in tp. The
 * timestamp value in 't' will will be the number of nanoseconds since the board was reset.
 * The timers on the board have only 32 bits and their resolution is in microseconds, so
 * the time returned will always be an even number of microseconds. Moreover, after about 71
 * minutes of operation, the timer overflows. 
 * 
 * The function reads out the upper word before and after reading the timer.
 * If the upper word has changed (i.e. it was an overflow in between),
 * we cannot simply combine them. We read once more to be sure that 
 * we read after the overflow.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set appropriately (see `man 2 clock_gettime`).
 */
int lf_clock_gettime(instant_t* t) {
    if (t == NULL) {
        // The t argument address references invalid memory
        // errno = EFAULT; //TODO: why does this not work with new build process?
        return -1;
    }
    uint32_t now_us_hi_pre = _lf_time_us_high;
    uint32_t now_us_low = nrfx_timer_capture(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL1);
    uint32_t now_us_hi_post = _lf_time_us_high; 

    // Check if we read the time during a wrap
    if (now_us_hi_pre != now_us_hi_post) {
        // There was a wrap. read again and return
        now_us_low = nrfx_timer_capture(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL1);
    }
    uint64_t now_us = COMBINE_HI_LO(now_us_hi_post, now_us_low);

    *t = ((instant_t)now_us) * 1000;

    return 0;
}

/**
 * @brief Return whether the critical section has been entered.
 * 
 * @return true if interrupts are currently disabled
 * @return false if interrupts are currently enabled
 */
bool in_critical_section() {
    // FIXME: if somehow interrupts get disabled directly (not through the NRF API),
    // then this will go undetected. A lower-level implementation that uses the ARM
    // instruction set directly would solve this problem.    
    if (nrf_nvic_state.__cr_flag != 0) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Pause execution for a given duration.
 * 
 * This implementation performs a busy-wait because it is unclear what will
 * happen if this function is called from within an ISR.
 * 
 * @param sleep_duration 
 * @return 0 for success, or -1 for failure.
 */
int lf_sleep(interval_t sleep_duration) {
    instant_t target_time;
    instant_t current_time;
    lf_clock_gettime(&current_time);
    target_time = current_time + sleep_duration;
    while (target_time <= current_time) {
        lf_clock_gettime(&current_time);
    }
    return 0;
}

/**
 * @brief Sleep until the given wakeup time.
 * 
 * @param wakeup_time The time instant at which to wake up.
 * @return int 0 if sleep completed, or -1 if it was interrupted.
 */
int lf_sleep_until(instant_t wakeup_time) {
    
    _lf_sleep_completed = false;

    uint32_t target_timer_val = (uint32_t)(wakeup_time / 1000);
    uint32_t curr_timer_val = nrfx_timer_capture(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL2);

    LF_PRINT_DEBUG("Entering suspend wait until t+%"PRIu32" us (current value: %"PRIu32")\n", target_timer_val, curr_timer_val);
    
    // assert that indeed we are in the critical section
    assert(in_critical_section());
    lf_critical_section_exit();

    // init timer interrupt for sleep time
    nrfx_timer_compare(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL2, target_timer_val, true);
    
    // wait for exception
    nrf_pwr_mgmt_run();
    
    // disable interrupt in case it is still pending
    nrfx_timer_compare_int_disable(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL2);

    lf_critical_section_enter();
    
    if (_lf_sleep_completed) {
        return 0;
    } else {
        LF_PRINT_DEBUG("Sleep got interrupted...\n");
        return -1;
    }
}

int lf_critical_section_enter() {
    // disable nvic
    sd_nvic_critical_region_enter(&_lf_nested_region);
    return 0;
}

int lf_critical_section_exit() {
    // enable nvic
    sd_nvic_critical_region_exit(_lf_nested_region);
    return 0;
}

/**
 * @brief Do nothing. On the NRF, sleep interruptions are recorded in
 * the function _lf_timer_event_handler. Whenever sleep gets interrupted,
 * the next function is re-entered to make sure the event queue gets 
 * checked again.
 * @return 0 
 */
int lf_notify_of_event() {
    // FIXME: record notifications so that we can immediately
    // restart the timer in case the interrupt was unrelated
    // to the scheduling of a new event.
   return 0;
}
