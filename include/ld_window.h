//
// Created by 邹嘉旭 on 2024/10/22.
//

#ifndef LD_WINDOW_H
#define LD_WINDOW_H
#include "../global/ldacs_sim.h"
#include "ld_buffer.h"

typedef struct window_item_s {
    uint8_t cos;
    uint8_t offset;
    buffer_t *buf;
    bool is_ack;
} window_item_t;


typedef struct window_pop_s {
    buffer_t *buf;
    uint8_t pid;
    uint8_t cos;
    uint16_t offset;
    bool is_rst;
    bool is_lfr;
}window_ctx_t;

/**
*
*/
typedef struct window_s{
    window_item_t *items;
    size_t  seq_sz,         /* the total size */
            avail_size,     /* the empty slot of window */
            win_size;       /* the window size */
    uint8_t to_ack_start,   /* the send-but-unacked start position */
            to_send_start,  /* the unsend-but-in-window start position */
            avail_start;    /* the first empty position */

    pthread_mutex_t *put_mutex;
    pthread_cond_t *put_cond;
}window_t;

window_t *init_window(size_t seq_size);

l_err window_put(window_t *w, uint8_t cos, buffer_t *buf, uint8_t *seq);

// window_item_t **get_items_in_window(window_t *w);

uint8_t get_window_end(window_t *w);


l_err window_put_ctx(window_t *w, window_ctx_t *pop);

window_ctx_t *check_pop_window_item(window_t *w, int64_t *avail_buf_sz);

l_err free_window_pop(window_ctx_t *pop);

#endif //LD_WINDOW_H
