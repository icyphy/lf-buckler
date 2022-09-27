/**
 * @file filter.h
 * @author Abhi Gundrala
 * @brief Header file with various filtering utilities for use in LF Labs.
 * @version 0.1
 */

#ifndef FILTER_H
#define FILTER_H

#include <stdio.h>
#include <stdlib.h>

/** Data Structures **/

/**
 * @brief Delay line struct implemented as a circular float buffer.
 */
typedef struct {
    float *head; // Pointer to start of buffer
    float *curr; // Pointer to n=0 sample in buffer
    size_t len; // Max length of buffer
} delay_line_t;

/** Functions **/

/**
 * @brief Allocate memory for a delay line of the specified length,
 * and initialize the `line` argument to refer to the allocated delay line.
 * 
 * @param line Pointer to a delay line struct to initialize
 * @param len Length of allocated buffer
 * @return -1 if `line` is null or len is 0
 */
int create_line(delay_line_t *line, size_t len);

/**
 * @brief Deallocate buffer for the delay line specified, 
 * and reset the struct pointers.
 * 
 * @param line Pointer to delay line struct
 * @return -1 if `line` is null
 */
int destroy_line(delay_line_t *line);

/**
 * @brief Append `x` to delay line,
 * and set current pointer to location of `x`.
 * Wrap the current pointer if it exceeds buffer limits.
 * 
 * @param line Pointer to delay line struct
 * @return -1 if `line` is null
 */
int push(delay_line_t *line, float x);

/**
 * Get the `n`-th most recent sample pushed into the specified delay line,
 * and return its value in the `x` argument.
 *
 * @param line Pointer to delay line struct
 * @param n Index of value to get
 * @return -1 if `n` is out of range.
 */
int get(delay_line_t *line, size_t n, float *x);

#endif