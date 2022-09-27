/**
 * dynamic size implementation of delay line
 * for signal processing applications.
 * Uses a software circular buffer as storage backend
 * @author Abhi Gundrala
 */

#include "filter.h"

int create_line(delay_line_t *line) {
    if (!line) return -1;
    if (line->size == 0) return -1;
    // allocate buffer
    line->head = (float *) calloc(line->size, sizeof(float));
    if (!line->head) return -1;
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
    line->size = 0;
    return 0;
}

int push(delay_line_t *line, float x) {
    float *tail;
    if (!line) return -1;
    // set value
    *line->curr = x;
    // increment pointer
    line->curr++;
    tail = line->head + line->size - 1;
    // wrap pointer if exceeds tail
    if (line->curr > tail) {
        line->curr = line->head;
    }
    return 0;
}

int get(delay_line_t *line, int t, float *x) {
    float *ptr;
    if (t >= line->size) return -1;
    // subtract offset from curr
    ptr = line->curr - t;
    // wrap if less than head
    if (ptr < line->head) {
        ptr = line->size + ptr;
    }
    // set value
    *x = *(ptr);
    return 0;
}

