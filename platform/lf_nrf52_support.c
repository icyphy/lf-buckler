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

#include "lf_nrf52_support.h"
#include "../platform.h"

#include "nrf_delay.h"
#include "nrf.h"
#include "nrf_drv_timer.h"
#include "app_error.h"

#ifdef NUMBER_OF_WORKERS
#endif

/**
 * Offset to _LF_CLOCK that would convert it
 * to epoch time.
 * For CLOCK_REALTIME, this offset is always zero.
 * For CLOCK_MONOTONIC, it is the difference between those
 * clocks at the start of the execution.
 */
interval_t _lf_epoch_offset = 0LL;

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
    // unimplemented - on the nrf, probably not possible to get a bearing on real time
}

/**
 * Initialize the LF clock.
 */
void lf_initialize_clock() {
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
 * timestamp value in 't' will always be epoch time, which is the number of
 * nanoseconds since January 1st, 1970.
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
    instant_t tp_in_us = (instant_t)(NRF_TIMER3->CC[1]);
    *t = tp_in_us * 1000 + _lf_epoch_offset;
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
 *
 * A Linux-specific clock_nanosleep is used underneath that is supposedly more
 * accurate.
 *
 * @return 0 for success, or -1 for failure. In case of failure, errno will be
 *  set appropriately (see `man 2 clock_nanosleep`).
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
            printf("DEBUG: INT RAISE \n");
            break;
        }
    }
    // disable interrupts
    head = int_list_head;
    while (head != NULL) {
        lf_mutex_lock((lf_mutex_t*)head);
        head = head->next;
    }
    // check if interrupted and return -1 on interrupt after wait
    // this will force the event queue to be checked again
    return INT_RAISED;
}
