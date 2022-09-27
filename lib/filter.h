/**
 * Filtering utils for use in lab lf programs
 * @author Abhi Gundrala
 */

#ifndef FILTER
#define FILTER

#include <stdio.h>
#include <stdlib.h>

/**
 * delay line struct
 * implemented as software circular buffer
 * uses floating point storage
 */
typedef struct _delay_line_t {
    float *head;
    float *curr;
    int size;
} delay_line_t;

/**
 * allocate memory buffer for delay line
 * and fill pointers in struct
 * return -1 on error
 */
int create_line(delay_line_t *line);

/**
 * deallocate buffer for delay line
 * and reset struct pointers
 * return -1 on error
 */
int destroy_line(delay_line_t *line);

/**
 * append value to delay line and set
 * current pointer to location of new value
 * wrap pointer if it exceeds buffer limits
 * return -1 on error
 */
int push(delay_line_t *line, float x);

/**
 * get sample x, t samples back from current
 * if t exceeds buffer size, return error code -1
 */
int get(delay_line_t *line, int t, float *x);

#endif