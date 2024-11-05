//
// Created by 邹嘉旭 on 2024/10/22.
//

#include "ld_window.h"


l_err free_window_item(window_item_t *wi);

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

window_item_t *pop_window_item(window_t *w) {
    if (w->avail_size == w->seq_sz)    return NULL;
    window_item_t *wi = calloc(1, sizeof(window_item_t));

    memcpy(wi, &w->items[w->win_start], sizeof(window_item_t));
    memset(&w->items[w->win_start], 0, sizeof(window_item_t));

    w->win_start = (w->win_start + 1) % w->seq_sz;
    w->avail_size++;

    pthread_cond_signal(w->put_cond);

    return wi;
}

window_item_t *pop_frag_window_item(window_t *w, size_t sz) {
    if (w->avail_size == w->seq_sz)    return NULL;
    window_item_t *wi = calloc(1, sizeof(window_item_t));
    window_item_t *p_item = &w->items[w->win_start % w->seq_sz];

    wi->cos = p_item->cos;
    wi->buf = init_buffer_ptr(sz);
    CLONE_TO_CHUNK(*wi->buf, p_item->buf->ptr, sz);

    p_item->is_frag = TRUE;

    return wi;
}


window_pop_t *check_pop_window_item(window_t *w, size_t *avail_buf_sz) {
    window_item_t *p_item = &w->items[w->win_start % w->seq_sz];
    if (p_item->buf == NULL || w->avail_size == 0)  return NULL;

    window_item_t *item = NULL;
    window_pop_t *pop_out = calloc(1, sizeof(window_pop_t));

    printf("%d %d\n", p_item->buf->len, *avail_buf_sz);
    if (p_item->buf->len <= *avail_buf_sz) {
        if ((item = pop_window_item(w)) == NULL) {
            free_window_item(item);
            return NULL;
        }
        pop_out->is_lfr = TRUE;

        *avail_buf_sz -= item->buf->len;

    } else {
        if ((item = pop_frag_window_item(w, *avail_buf_sz)) == NULL) {
            free_window_item(item);
            return NULL;
        }
        pop_out->is_lfr = FALSE;
    }
    pop_out->cos = item->cos;
    pop_out->buf = init_buffer_unptr();
    CLONE_BY_BUFFER(*pop_out->buf, *item->buf);
    pop_out->is_rst = item->is_frag == TRUE ?  FALSE : TRUE;

    free_window_item(item);

    return pop_out;
}

l_err free_window_item(window_item_t *wi) {
    if (wi) {
        free_buffer(wi->buf);
        free(wi);
    }
    return LD_OK;
}

l_err free_window_pop(window_pop_t *pop) {
    if (pop) {
        free_buffer(pop->buf);
        free(pop);
    }
    return LD_OK;
}