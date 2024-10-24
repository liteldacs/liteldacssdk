//
// Created by 邹嘉旭 on 2024/10/22.
//

#include "ld_window.h"


uint8_t get_window_end(window_t *w){
    return (w->win_start + w->win_size) % w->seq_sz;
}

window_t *init_window(size_t seq_size){
    window_t *w = malloc(sizeof(window_t));
    w->items = calloc(seq_size, sizeof(window_item_t));
    w->seq_sz = w->avail_size = seq_size;
    w->win_size = seq_size >> 1;

    w->win_start = w->avail_start = 0;

    w->put_mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
    if (w->put_mutex == NULL) {
        free(w->items);
        free(w);
        return NULL;
    }
    pthread_mutex_init(w->put_mutex, NULL);

    w->put_cond = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    if (w->put_cond == NULL) {
        pthread_mutex_destroy(w->put_mutex);
        free(w->put_mutex);
        free(w->items);
        free(w);
        return NULL;
    }
    pthread_cond_init(w->put_cond, NULL);
    return w;
}

l_err slide_window(window_t *w, size_t sl_size) {
    w->win_start = (w->win_start + sl_size) % w->seq_sz;
    return LD_OK;
}

window_item_t **get_items_in_window(window_t *w){
    window_item_t **items_ptr = calloc(w->win_size, sizeof(void *));

    for (int i = 0; i < w->win_size; i++){
        items_ptr[i] = &w->items[(w->win_start + i) % w->seq_sz];
    }
    return items_ptr;
}

l_err put_window_item(window_t *w, uint8_t cos, buffer_t *buf, uint8_t *seq){
    if (w->avail_size == 0) return LD_ERR_INTERNAL;
    w->items[w->avail_start].cos = cos;
    w->items[w->avail_start].buf = buf;

    *seq = w->avail_start;

    w->avail_start = (w->avail_start + 1) % w->seq_sz;
    w->avail_size--;

    return LD_OK;
}

l_err pop_window_item(window_t *w, window_item_t **item) {
    if (w->win_size == 0)    return LD_ERR_INTERNAL;
    *item = &w->items[w->win_start];
    w->win_start = (w->win_start + 1) % w->seq_sz;
    w->avail_size++;

    pthread_cond_signal(w->put_cond);

    return LD_OK;
}
