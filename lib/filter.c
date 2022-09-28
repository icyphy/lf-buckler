/**
 * @file filter.c
 * @author Abhi Gundrala
 * @brief Implementation of various filtering utilities for use in LF Labs.
 */

#include "filter.h"
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

int create_line(delay_line_t *line, size_t len) {
    if (!line) return -1;
    // allocate buffer
    line->head = (float *) calloc(len, sizeof(float));
    if (!line->head) return -1;
    line->len = len;
    // set curr pointer to start of buffer
    line->curr = line->head;
    return 0;
}

int destroy_line(delay_line_t *line) {
    if (!line) return -1;
    // free buffer
    free(line->head);
    // set struct to empty
    line->head = NULL;
    line->curr = NULL;
    line->len = 0;
    return 0;
}

int push(delay_line_t *line, float x) {
    float *tail;
    if (!line) return -1;
    // set value
    *line->curr = x;
    // increment pointer
    line->curr++;
    tail = line->head + line->len - 1;
    // wrap pointer if exceeds tail
    if (line->curr > tail) {
        line->curr = line->head;
    }
    return 0;
}

int get(delay_line_t *line, size_t n, float *x) {
    float *ptr;
    if (n >= line->len) return -1;
    // subtract offset from curr
    ptr = line->curr - n;
    // wrap if less than head
    if (ptr < line->head) {
        ptr = line->len + ptr;
    }
    // set value
    *x = *(ptr);
    return 0;
}

float sum_line(delay_line_t *line) {
    // take avg of values in line
    float sum = 0;
    float* ptr = line->head;
    for (size_t i=0; i < line->len; i++) {
        sum += ptr[i];
    }
    return sum;
}

float fir_filter(delay_line_t *line, float *b, size_t b_size) {
    // perform convolution
    // zero pad h to fit size of x
    float xi;
    size_t n = min(line->len, b_size);
    float sum = 0;
    for (size_t i = 0; i < n; i++) {
        // get the ith value before current
        get(line, i, &xi);
        sum += b[i] * xi;
    }
    return sum;
}