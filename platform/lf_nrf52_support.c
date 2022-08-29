/* MacOS API support for the C target of Lingua Franca. */

/*************
Copyright (c) 2021, The University of California at Berkeley.

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

/** NRF52 support for the C target of Lingua Franca.
 *  Interrupt only support, does not implement multi-threading
 *  or realtime clock support.
 *
 *  @author{Soroush Bateni <soroush@utdallas.edu>}
 *  @author{Abhi Gundrala <gundralaa@berkeley.edu>}
 */

#include <stdlib.h> // Defines malloc.
#include <string.h> // Defines memcpy.

#include "lf_nrf52_support.h"
#include "../platform.h"

#include "nrf.h"
#include "nrfx_timer.h"
#include "nrf_nvic.h"
#include "nrf_soc.h"
#include "app_error.h"

#ifdef NUMBER_OF_WORKERS
#endif

#include <stdarg.h>
#include <stdio.h>

void guarded_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    // Disable interrupts.
    uint8_t region = 0;
    sd_nvic_critical_region_enter(&region);
    vprintf(fmt, args);
    // Reenable interrupts if they were enabled.
    sd_nvic_critical_region_exit(region);
    va_end(args);
}

/**
 * lf global timer instance
 * timerId = 3 TIMER3
 */
static const nrfx_timer_t g_lf_timer_inst = NRFX_TIMER_INSTANCE(3);

/**
 * Keep track of interrupts being raised.
 * Allow sleep to exit with nonzero return on interrupt.
 */
bool _lf_timer_interrupted = false;
bool _lf_overflow_corrected = false;

// FIXME: Remove this. See other FIXMEs associated with it.
// uint8_t _lf_nested_region = 0;

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
 * To handle overflow of a 32-bit timer with microsecond resolution,
 * record the previous query to the timer here.
 */
uint32_t _lf_previous_timer_time = 0u;

/**
 * Handles LF timer interrupts
 * Using lf_timer instance -> id = 3
 * channel2 -> channel for lf_nanosleep interrupt
 * channel3 -> channel for overflow interrupt
 *
 * [in] event_type
 *      channel that fired interrupt on timer
 * [in] p_context
 *      context passed to handler
 * 
 */
void lf_timer_event_handler(nrf_timer_event_t event_type, void *p_context) {
    
    // check if event triggered on channel 2
    // sleep handle
    if (event_type == NRF_TIMER_EVENT_COMPARE2) {
        _lf_timer_interrupted = true;
    }

    // check if event triggered on channel 3
    // overflow handle
    if (event_type == NRF_TIMER_EVENT_COMPARE3) {
        if (!_lf_overflow_corrected) {
            _lf_time_epoch_offset += (1LL << 32) * 1000;
        }
    }
    _lf_overflow_corrected = false;
}

/**
 * Initialize the LF clock.
 */
void lf_initialize_clock() {
    _lf_time_epoch_offset = 0LL;
    _lf_previous_timer_time = 0u;

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

    nrfx_timer_init(&g_lf_timer_inst, &timer_conf, &lf_timer_event_handler);
    
    // Enable an interrupt to occur on channel NRF_TIMER_CC_CHANNEL3
    // when the timer reaches its maximum value and is about to overflow.
    nrfx_timer_compare(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL3, ~0x0, true);
    nrfx_timer_enable(&g_lf_timer_inst);

    // FIXME: The following call disables interrupts.
    // However, this causes any blocking I/O function, where a sensor is read
    // or an actuator driven by code that waits until the I/O operation completes,
    // to deadlock.  Most of the I/O in the labs is based on such blocking calls.
    // As a consequence, any asynchronous call to lf_schedule() runs the risk
    // of corrupting the event queue!  This is not safe.
    // The solution is to fix the unthreaded C runtime to use mutexes.
    // sd_nvic_critical_region_enter(&_lf_nested_region);
}

/**
 * Fetch the value of _LF_CLOCK (see lf_linux_support.h) and store it in tp. The
 * timestamp value in 't' will will be the number of nanoseconds since the board was reset.
 * The timers on the board have only 32 bits and their resolution is in microseconds, so
 * the time returned will always be an even number of microseconds. Moreover, after about 71
 * minutes of operation, the timer overflows. To correct for this, this function assumes that
 * if the time returned by the timer is less than what it returned on the previous call, then
 * a single overflow has occurred. The function therefore adds about 71 minutes to the time
 * returned.  This will work correctly as long as this function is called at intervals that
 * do not exceed 71 minutes. 
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
    uint32_t curr_timer_time;
    // capture latest timer value
    curr_timer_time = nrfx_timer_capture(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL1);
    
    // in case overflow interrupt was missed
    if (_lf_previous_timer_time > curr_timer_time) {
        _lf_time_epoch_offset += (1LL << 32) * 1000;
        _lf_overflow_corrected = true;
    }
    *t = ((instant_t)curr_timer_time) * 1000 + _lf_time_epoch_offset;
    _lf_previous_timer_time = curr_timer_time;
    return 0;
}

/**
 * Pause execution for a number of nanoseconds.
 * This implementation busy waits until the time reported by lf_clock_gettime()
 * elapses by more than the requested time.
 *
 * @return 0 for success, or -1 if interrupted.
 */
int lf_nanosleep(instant_t requested_time) {
    uint32_t target_timer_val;
    instant_t target_time;

    lf_clock_gettime(&target_time);
    target_time += requested_time;
    target_timer_val = (requested_time - _lf_time_epoch_offset) / 1000;

    // timer interrupt flag default false
    // callback fires and asserts bool
    _lf_timer_interrupted = false;
    // init timer interrupt for sleep time
    nrfx_timer_compare(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL2, target_timer_val, true);
    
    // FIXME: Remove this. See other FIXMEs associated with it.
    // enable nvic
    // sd_nvic_critical_region_exit(_lf_nested_region);
    // wait for interrupt
    sd_app_evt_wait();
    // FIXME: Remove this. See other FIXMEs associated with it.
    // disable nvic
    // sd_nvic_critical_region_enter(&_lf_nested_region);
    
    int result = (_lf_timer_interrupted) ? 0 : -1;
    // Check whether timer interrupted and return -1 on nont timer interrupt.
    // This will force the event queue to be checked again.
    return result;
}
