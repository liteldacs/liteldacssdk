//
// Created by 邹嘉旭 on 2024/10/22.
//

#ifndef LD_WINDOW_H
#define LD_WINDOW_H
#include "../global/ldacs_sim.h"
#include "ld_buffer.h"

typedef struct window_item_s {
    uint8_t seq;
    uint8_t cos;
    buffer_t *buf;
} window_item_t;

typedef struct window_s{
    window_item_t *items;
    size_t seq_sz, avail_size, win_size;
    uint8_t win_start, avail_start;

    pthread_mutex_t *put_mutex;
    pthread_cond_t *put_cond;
}window_t;

window_t *init_window(size_t seq_size);

l_err put_window_item(window_t *w, uint8_t cos, buffer_t *buf, uint8_t *seq);

uint8_t get_window_end(window_t *w);

#endif //LD_WINDOW_H
