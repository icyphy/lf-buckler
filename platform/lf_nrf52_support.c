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

    #include "nrf_gpio.h"   

#ifdef NUMBER_OF_WORKERS
#endif

    #define PIN1 NRF_GPIO_PIN_MAP(0,27)
    #define PIN2 NRF_GPIO_PIN_MAP(0, 26)
   #define PIN3 NRF_GPIO_PIN_MAP(0, 25)
   #define PIN4 NRF_GPIO_PIN_MAP(0, 24)

    #define LED0 NRF_GPIO_PIN_MAP(0,17)
    #define LED1 NRF_GPIO_PIN_MAP(0,18)
    #define LED3 NRF_GPIO_PIN_MAP(0, 20)
/**
 * lf global timer instance
 * timerId = 3 TIMER3
 */
#define LF_TIMER_ID 3
static const nrfx_timer_t g_lf_timer_inst = NRFX_TIMER_INSTANCE(LF_TIMER_ID);

#define COMBINE_HI_LO(hi,lo) ((((uint64_t) hi) << 32) | ((uint64_t) lo))
#define MAX_SLEEP_NS 1000ULL*UINT32_MAX

/**
 * 

/**
 * Keep track of interrupts being raised.
 * Allow sleep to exit with nonzero return on interrupt.
 */
bool _lf_timer_interrupted = false;
uint8_t _lf_nested_region = 0;

/**
 * Offset to _LF_CLOCK that would convert it
 * to epoch time.
 * For CLOCK_REALTIME, this offset is always zero.
 * For CLOCK_MONOTONIC, it is the difference between those
 * clocks at the start of the execution.
 */
uint32_t _lf_time_us_high = 0;
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
        nrf_gpio_pin_set(PIN3);
    
    
    // Channel 2 = Timeout on sleep timer
    if (event_type == NRF_TIMER_EVENT_COMPARE2) {
        _lf_timer_interrupted = true;
    }

    // check if event triggered on channel 3
    // overflow handle
    if (event_type == NRF_TIMER_EVENT_COMPARE3) {
        _lf_time_us_high += 1;
    }
    nrf_gpio_pin_clear(PIN3);
}

/**
 * Initialize the LF clock.
 */
void lf_initialize_clock() {

    nrf_gpio_cfg_output(PIN1);
    nrf_gpio_cfg_output(PIN2);
    nrf_gpio_cfg_output(PIN3);
    nrf_gpio_cfg_output(PIN4);

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
}

/**
 * Fetch the value of _LF_CLOCK (see lf_linux_support.h) and store it in tp. The
 * timestamp value in 't' will will be the number of nanoseconds since the board was reset.
 * By reading out the hi-word of the time before and after accessing the timer we know
 * wether there occurred an timer-overflow while reading. If so, read once again and use latest value
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
 * Pause execution for a number of nanoseconds.
 * This implementation busy waits until the time reported by lf_clock_gettime()
 * elapses by more than the requested time.
 *
 * @return 0 for success, or -1 if interrupted.
 */
int lf_nanosleep(instant_t requested_sleep_ns) {
    nrf_gpio_pin_set(PIN1);

    // If sleep request is too high. Return -1 until it is small enough
    //  in practice a busy wait until the sleep is less than an overflow period of the timer
    if (requested_sleep_ns > MAX_SLEEP_NS) {
        return -1;
    }


    uint32_t requested_sleep_us = (uint32_t) (requested_sleep_ns/1000);
    uint32_t now_us = nrfx_timer_capture(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL2);
    uint32_t sleep_until_us = requested_sleep_us + now_us;

    // timer interrupt flag default false
    // callback fires and asserts bool
    _lf_timer_interrupted = false;
    // init timer interrupt for sleep time
    printf("now: %lu request: %lli sleep_until: %lu \n", now_us, requested_sleep_ns, sleep_until_us);
    nrfx_timer_compare(&g_lf_timer_inst, NRF_TIMER_CC_CHANNEL2, sleep_until_us, true);
 
    // enable nvic
    //sd_nvic_critical_region_exit(_lf_nested_region);
    // wait for interrupt
    nrf_gpio_pin_set(PIN3);
    __WFE();
    nrf_gpio_pin_clear(PIN3);
    // disable nvic
    //sd_nvic_critical_region_enter(&_lf_nested_region);

    int result = (_lf_timer_interrupted) ? 0 : -1;

    nrf_gpio_pin_clear(PIN1);
    return result;
}
