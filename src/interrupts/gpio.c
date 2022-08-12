// Empty function definitions.
// Replace these with your design from Chapter 4.

#include "gpio.h"

// Inputs: 
//  gpio_num - gpio number 0-31
//  dir - gpio direction (INPUT, OUTPUT)
void gpio_config(uint8_t gpio_num, gpio_direction_t dir) {

}

// Set gpio_num high
// Inputs: 
//  gpio_num - gpio number 0-31
void gpio_set(uint8_t gpio_num) {

}

// Set gpio_num low
// Inputs: 
//  gpio_num - gpio number 0-31
void gpio_clear(uint8_t gpio_num) {

}

// Inputs: 
//  gpio_num - gpio number 0-31
bool gpio_read(uint8_t gpio_num) {
    // should return pin state
    return 0;
}
