//
// Created by 邹嘉旭 on 2024/10/22.
//

#include "ld_window.h"

#include <ld_log.h>


l_err free_window_item(window_item_t *wi);

uint8_t get_window_end(window_t *w){
    return (w->to_ack_start + w->win_size) % w->seq_sz;
}

window_t *init_window(size_t seq_size){
    window_t *w = malloc(sizeof(window_t));
    w->items = calloc(seq_size, sizeof(window_item_t));
    w->seq_sz = w->avail_size = seq_size;
    w->win_size = seq_size >> 1;

    w->to_ack_start = w->to_send_start = w->avail_start = 0;

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

l_err window_put(window_t *w, uint8_t cos, buffer_t *buf, uint8_t *seq){
    if (w->avail_size == 0) return LD_ERR_INTERNAL;
    w->items[w->avail_start].cos = cos;
    w->items[w->avail_start].buf = buf;
    w->items[w->avail_start].is_ack = FALSE;

    *seq = w->avail_start;

    w->avail_start = (w->avail_start + 1) % w->seq_sz;
    w->avail_size--;

    return LD_OK;
}

window_item_t *pop_window_item(window_t *w) {
    if (w->avail_size == w->seq_sz)    return NULL;
    window_item_t *wi = calloc(1, sizeof(window_item_t));
    window_item_t *p_item = &w->items[w->to_ack_start % w->seq_sz];

    p_item->offset = p_item->buf->len;

    memcpy(wi, p_item, sizeof(window_item_t));
    memset(p_item, 0, sizeof(window_item_t));

    w->to_ack_start = (w->to_ack_start + 1) % w->seq_sz;
    w->avail_size++;

    pthread_cond_signal(w->put_cond);

    return wi;
}

window_item_t *pop_frag_window_item(window_t *w, size_t sz) {
    if (w->avail_size == w->seq_sz)    return NULL;
    window_item_t *wi = calloc(1, sizeof(window_item_t));
    window_item_t *p_item = &w->items[w->to_ack_start % w->seq_sz];


    wi->cos = p_item->cos;
    wi->buf = init_buffer_ptr(sz);
    CLONE_TO_CHUNK(*wi->buf, p_item->buf->ptr + p_item->offset, sz);

    p_item->offset += sz;
    wi->offset = p_item->offset;

    return wi;
}


l_err window_put_ctx(window_t *w, window_ctx_t *pop) {
    window_item_t *p_item = &w->items[pop->pid];
    // if(p_item->buf == NULL) {
    //     log_error("Wrong Window Item by %d", pop->pid);
    //     return LD_ERR_NULL;
    // }

    p_item->offset = pop->offset;
    p_item->cos = pop->cos;
    if(pop->is_rst == TRUE) {
        //offset is 0
        p_item->buf = init_buffer_unptr();
        CLONE_TO_CHUNK(*p_item->buf, pop->buf->ptr, pop->buf->len);
    }else {
        if(p_item->buf == NULL) {
            log_error("Fragment buffer is Null");
            return LD_ERR_NULL;
        }
        if(cat_to_buffer(p_item->buf, pop->buf->ptr, pop->buf->len)) {
            log_error("CAT BUFFER Failed");
            return LD_ERR_INTERNAL;
        }
    }
    return LD_OK;
}

window_ctx_t *check_pop_window_item(window_t *w, int64_t *avail_buf_sz) {
    window_item_t *p_item = &w->items[w->to_ack_start % w->seq_sz];
    if (p_item->buf == NULL || w->avail_size == 0)  return NULL;

    window_ctx_t *pop_out = calloc(1, sizeof(window_ctx_t));
    pop_out->pid = w->to_ack_start;
    pop_out->is_rst = p_item->offset != 0 ?  FALSE : TRUE;

    window_item_t *item = NULL;

    if (p_item->buf->len <= *avail_buf_sz) {
        if ((item = pop_window_item(w)) == NULL) {
            free_window_item(item);
            return NULL;
        }
        pop_out->is_lfr = TRUE;

        *avail_buf_sz -= (int64_t)item->buf->len;

    } else {
        if ((item = pop_frag_window_item(w, *avail_buf_sz)) == NULL) {
            free_window_item(item);
            return NULL;
        }
        pop_out->is_lfr = FALSE;
    }

    log_warn("!!!!!!! OFFSET %d", item->offset);
    pop_out->cos = item->cos;
    pop_out->buf = init_buffer_unptr();
    CLONE_BY_BUFFER(*pop_out->buf, *item->buf);
    pop_out->offset = item->offset;

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

l_err free_window_pop(window_ctx_t *pop) {
    if (pop) {
        free_buffer(pop->buf);
        free(pop);
    }
    return LD_OK;
}