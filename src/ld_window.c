//
// Created by 邹嘉旭 on 2024/10/22.
//

#include "ld_window.h"

#include <ld_log.h>


static l_err free_window_item(window_item_t *wi);
static l_err clear_window_item(window_item_t *wi);

uint8_t window_end(window_t *w){
    return (w->to_ack_start + w->win_size) % w->seq_sz;
}

window_t *init_window(size_t seq_size){
    window_t *w = malloc(sizeof(window_t));
    // if (!)
    w->items = calloc(seq_size, sizeof(window_item_t));
    w->seq_sz = w->avail_size = seq_size;
    w->win_size = seq_size >> 1;

    w->to_ack_start = w->to_send_start = w->avail_start = w->to_recv_start = 0;

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

l_err window_destory(window_t *w) {
    if (!w) return LD_ERR_NULL;
    if (w->items) free(w->items);
    free(w);
    return LD_OK;
}

l_err window_put(window_t *w, uint8_t cos, buffer_t *buf, uint8_t *seq){
    // if (w->avail_size == 0) return LD_ERR_INTERNAL;
    if (w->avail_size == 0) pthread_cond_wait(w->put_cond, w->put_mutex);

    window_item_t *p_item = &w->items[w->avail_start];

    p_item->cos = cos;
    p_item->buf = init_buffer_unptr();
    p_item->is_ack = FALSE;

    CLONE_BY_BUFFER(*w->items[w->avail_start].buf, *buf);

    *seq = w->avail_start;

    w->avail_start = (w->avail_start + 1) % w->seq_sz;
    w->avail_size--;

    return LD_OK;
}

l_err window_put_ctx(window_t *w, window_ctx_t *ctx) {
    uint8_t win_end = (w->to_recv_start + w->win_size) % w->seq_sz;

    if (!(ctx->pid >= w->to_recv_start && ctx->pid < win_end) && !(win_end  < w->to_recv_start && (ctx->pid < win_end || ctx->pid >= w->to_recv_start))) {
        log_error("Can not input context into windows %d %d %d %d", ctx->pid, w->to_recv_start, win_end, w->to_send_start);
        return LD_ERR_INVALID;
    }

    window_item_t *p_item = &w->items[ctx->pid];

    p_item->offset = ctx->offset;
    p_item->cos = ctx->cos;
    if(ctx->is_rst == TRUE) {
        //offset is 0
        p_item->buf = init_buffer_unptr();
        CLONE_TO_CHUNK(*p_item->buf, ctx->buf->ptr, ctx->buf->len);
    }else {
        if(p_item->buf == NULL) {
            log_error("Fragment buffer is Null. PID is `%d`", ctx->pid);
            return LD_ERR_NULL;
        }
        if(cat_to_buffer(p_item->buf, ctx->buf->ptr, ctx->buf->len)) {
            log_error("CAT BUFFER Failed");
            return LD_ERR_INTERNAL;
        }
    }

    p_item->all_recv = ctx->is_lfr;
    p_item->is_processed = FALSE;

    w->avail_size--;

    return LD_OK;
}



buffer_t *window_in_get(window_t *w) {
    buffer_t *buf = NULL;
    for (int i = 0; i < w->win_size; i++) {
        window_item_t *p_item = &w->items[(w->to_recv_start + i) % w->seq_sz];
        if (p_item->all_recv == TRUE && p_item->is_processed == FALSE) {
            buf = calloc(1, sizeof(buffer_t));
            CLONE_BY_BUFFER(*buf, *p_item->buf);

            w->avail_size++;
            p_item->is_processed = TRUE;
            break;
        }
    }

    /* slide window */
    uint8_t to_recv = w->to_recv_start;
    while (w->items[to_recv].is_processed == TRUE) {
        clear_window_item(&w->items[to_recv]);
        log_warn("IN GET %d %d", to_recv, w->seq_sz);
        to_recv = (to_recv + 1 ) % w->seq_sz;
        w->to_recv_start = to_recv;
    }

    return buf;
}


static window_item_t *window_out_get(window_t *w) {
    if (w->avail_size == w->seq_sz)    return NULL;
    window_item_t *wi = calloc(1, sizeof(window_item_t));
    window_item_t *p_item = &w->items[w->to_send_start % w->seq_sz];

    p_item->offset = p_item->buf->len;

    memcpy(wi, p_item, sizeof(window_item_t));
    wi->buf = init_buffer_unptr();
    CLONE_BY_BUFFER(*wi->buf, *p_item->buf);

    w->to_send_start = (w->to_send_start + 1) % w->seq_sz;
    w->avail_size++;

    pthread_cond_signal(w->put_cond);

    return wi;
}


static window_item_t *window_out_get_frag(window_t *w, size_t sz) {
    if (w->avail_size == w->seq_sz)    return NULL;
    window_item_t *wi = calloc(1, sizeof(window_item_t));
    window_item_t *p_item = &w->items[w->to_send_start % w->seq_sz];

    wi->cos = p_item->cos;
    wi->buf = init_buffer_ptr(sz);
    CLONE_TO_CHUNK(*wi->buf, p_item->buf->ptr + p_item->offset, sz);

    p_item->offset += sz;
    wi->offset = p_item->offset;

    return wi;
}

window_ctx_t *window_check_get(window_t *w, int64_t *avail_buf_sz) {
    window_item_t *p_item = &w->items[w->to_send_start % w->seq_sz];
    if (p_item->buf == NULL) {
        return NULL;
    }

    window_ctx_t *pop_out = calloc(1, sizeof(window_ctx_t));
    pop_out->pid = w->to_send_start;
    pop_out->is_rst = p_item->offset != 0 ?  FALSE : TRUE;

    window_item_t *item = NULL;

    if (p_item->buf->len <= *avail_buf_sz) {
        if ((item = window_out_get(w)) == NULL) {
            free_window_ctx(pop_out);
            free_window_item(item);
            return NULL;
        }
        pop_out->is_lfr = TRUE;

        *avail_buf_sz -= (int64_t)item->buf->len;

    } else {
        if ((item = window_out_get_frag(w, *avail_buf_sz)) == NULL) {
            free_window_ctx(pop_out);
            free_window_item(item);
            return NULL;
        }
        pop_out->is_lfr = FALSE;
    }

    pop_out->cos = item->cos;
    pop_out->buf = init_buffer_unptr();
    CLONE_BY_BUFFER(*pop_out->buf, *item->buf);
    pop_out->offset = item->offset;

    free_window_item(item);

    return pop_out;
}


static void window_slide(window_t *w) {
    while ((w->to_ack_start < w->to_send_start) ||
        (w->to_send_start < w->to_ack_start && (w->to_ack_start > (w->to_send_start + w->win_size) ))) {
        window_item_t *p_item = &w->items[w->to_ack_start];
        if (p_item->is_ack == TRUE) {
            free_buffer(p_item->buf);
            memset(p_item, 0, sizeof(window_item_t));
            w->to_ack_start =  (w->to_ack_start + 1) % w->seq_sz;
        }else {
            break;
        }
    }
}

l_err window_ack_item(window_t *w, uint8_t PID) {
    if ((PID >= w->to_ack_start && PID < w->to_send_start) ||
        (w->to_send_start < w->to_ack_start && (PID >= w->to_ack_start || PID < w->to_send_start))) {
        window_item_t *p_item = &w->items[PID];
        p_item->is_ack = TRUE;

        window_slide(w);
        return LD_OK;
    }
    return LD_ERR_INVALID;
}


l_err window_set_send_start(window_t *w, uint8_t pos) {
    if (!w) return LD_ERR_NULL;
    w->to_send_start = w->avail_start = pos;
    w->to_ack_start = pos;
    return LD_OK;
}

static l_err free_window_item(window_item_t *wi) {
    if (wi) {
        free_buffer(wi->buf);
        free(wi);
    }
    return LD_OK;
}

static l_err clear_window_item(window_item_t *wi) {
    free_buffer(wi->buf);
    memset(wi, 0, sizeof(window_item_t));
    return LD_OK;
}

l_err free_window_ctx(window_ctx_t *ctx) {
    if (ctx) {
        free_buffer(ctx->buf);
        free(ctx);
    }
    return LD_OK;
}