/* nRF52832 API support for the C target of Lingua Franca. */

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

/** nrf52 API support for the C target of Lingua Franca.
 *  
 *  @author{Soroush Bateni <soroush@utdallas.edu>}
 *  @author{Abhi Gundrala <gundralaa@berkeley.edu>}
 */

#ifndef LF_NRF52_SUPPORT_H
#define LF_NRF52_SUPPORT_H

#include <stdint.h> // For fixed-width integral types
#include <time.h>   // For CLOCK_MONOTONIC
#include <stdbool.h>


#ifdef NUMBER_OF_WORKERS

// typedef nrf_mtx_t _lf_mutex_t;
// typedef CONDITION_VARIABLE _lf_cond_t;
// typedef HANDLE _lf_thread_t;

#endif

/**
 * For the nrf52, each mutex will control an interrupt.
 *
 * The mutex holds the interrupt number.
 * For example, a mutex might be defined for the GPIOTE peripheral interrupt number
 * 
 * When initialized, the interrupt is inserted into a global linked list
 * for disabling and enabling all interrupts during sleep functions.
 * - All interrupts are disabled by default after initialization
 * - Priority levels are restricted between (0-7)
 * 
 */

typedef struct nrf_int {
    uint8_t int_num;
    uint8_t priority;
    struct nrf_int* next;
} nrf_int;

typedef nrf_int _lf_mutex_t;

// TODO: find a better way to implement this
/**
 * Keeps track of interrupts being raised
 * Allows sleep to exit with nonzero return on interrupt
 */
extern uint8_t INT_RAISED;

/**
 * Time instant. Both physical and logical times are represented
 * using this typedef.
 * WARNING: If this code is used after about the year 2262,
 * then representing time as a 64-bit long long will be insufficient.
 */
typedef int64_t _instant_t;

/**
 * Interval of time.
 */
typedef int64_t _interval_t;

/**
 * Microstep instant.
 */
typedef uint32_t _microstep_t;


/**
 * For user-friendly reporting of time values, the buffer length required.
 * This is calculated as follows, based on 64-bit time in nanoseconds:
 * Maximum number of weeks is 15,250
 * Maximum number of days is 6
 * Maximum number of hours is 23
 * Maximum number of minutes is 59
 * Maximum number of seconds is 59
 * Maximum number of nanoseconds is 999,999,999
 * Maximum number of microsteps is 4,294,967,295
 * Total number of characters for the above is 24.
 * Text descriptions and spaces add an additional 55,
 * for a total of 79. One more allows for a null terminator.
 */
#define LF_TIME_BUFFER_LENGTH 80


// The underlying physical clock for Linux
#define _LF_CLOCK CLOCK_MONOTONIC

#endif // LF_nRF52832_SUPPORT_H