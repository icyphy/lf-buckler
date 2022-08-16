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

#include "nrf_delay.h"
#include "nrf.h"
#include "nrf_drv_timer.h"
#include "app_error.h"
#include <stdlib.h>
#include <string.h>

#ifdef NUMBER_OF_WORKERS
#endif

uint8_t INT_RAISED = 0;

/**
 * Keep track of interrupts being raised.
 * Allow sleep to exit with nonzero return on interrupt.
 */
uint8_t INT_RAISED;

/**
 * Offset to _LF_CLOCK that would convert it
 * to epoch time.
 * For CLOCK_REALTIME, this offset is always zero.
 * For CLOCK_MONOTONIC, it is the difference between those
 * clocks at the start of the execution.
 */
interval_t _lf_time_epoch_offset = 0LL;

// Interrupt list pointers
nrf_int* int_list_head = NULL;
nrf_int* int_list_cur = NULL;



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
    
    NRF_TIMER3->BITMODE = 3;
    NRF_TIMER3->PRESCALER = 4;
    NRF_TIMER3->TASKS_CLEAR = 1;
    NRF_TIMER3->TASKS_START = 1;
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
    
    NRF_TIMER3->TASKS_CAPTURE[1] = 1;

    // Handle possible overflow.
    uint32_t current_timer_time = NRF_TIMER3->CC[1];
    if (current_timer_time < _lf_previous_timer_time) {
        // Overflow has occurred. Use _lf_time_epoch_offset to correct.
        _lf_time_epoch_offset += (1LL << 32) * 1000;
    }

    *t = ((instant_t)current_timer_time) * 1000 + _lf_time_epoch_offset;
    _lf_previous_timer_time = current_timer_time;
    return 0;
}

/**
 * Lock a mutex.
 * Disable a specific interrupt number on NVIC.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_lock(lf_mutex_t* mutex) {
    if (mutex == NULL) {
        return -1;
    }
    NVIC_DisableIRQ(mutex->int_num);
    return 0;
}

/** 
 * Unlock a mutex.
 * Enable a specific interrupt number on NVIC.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_unlock(lf_mutex_t* mutex) {
    if (mutex == NULL) {
        return -1;
    }
    NVIC_EnableIRQ(mutex->int_num);
    return 0;
}

/**
 * Initialize a mutex.
 * Set priority of interrupt number and enable.
 * 
 * @return 0 on success, platform-specific error number otherwise.
 */
int lf_mutex_init(lf_mutex_t* mutex) {
    if (mutex == NULL) {
        return -1;
    }
    NVIC_SetPriority(mutex->int_num, mutex->priority);
    lf_mutex_lock(mutex);
    // add to interrupt to global int list
    nrf_int* cpy_int = (nrf_int*) malloc(sizeof(nrf_int));
    memcpy(cpy_int, (nrf_int*)mutex, sizeof(nrf_int));
    int_list_cur->next = cpy_int;
    int_list_cur = cpy_int;
    if (int_list_head == NULL) {
        int_list_head = int_list_cur;
    }
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
    instant_t target_time;
    instant_t cur_time;
    lf_clock_gettime(&target_time);
    target_time += requested_time;

    // enable all interrupts
    nrf_int* head;
    head = int_list_head;
    while (head != NULL) {
        lf_mutex_unlock((lf_mutex_t*)head);
        head = head->next;
    }

    INT_RAISED = 0;
    while(cur_time <= target_time) {
        lf_clock_gettime(&cur_time);
        if (INT_RAISED != 0) {
            printf("DEBUG: Interrupt raised during lf_nanosleep.\n");
            break;
        }
    }
    // disable interrupts
    head = int_list_head;
    while (head != NULL) {
        lf_mutex_lock((lf_mutex_t*)head);
        head = head->next;
    }
    // Check whether interrupted and return -1 on interrupt after wait.
    // This will force the event queue to be checked again.
    return INT_RAISED;
}
