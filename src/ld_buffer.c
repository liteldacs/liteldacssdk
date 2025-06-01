//
// Created by jiaxv on 23-7-2.
//

#include "ld_buffer.h"

#include <ld_log.h>

uint8_t *alloc_buffer(size_t size) {
    uint8_t *p = (uint8_t *) calloc(size + 1, sizeof(uint8_t));
    if (p == NULL) {
        return NULL;
    }
    p[size] = '\0';
    return p;
}

buffer_t *init_buffer_unptr() {
    buffer_t *p = malloc(sizeof(buffer_t));
    p->len = p->total = p->free = 0;
    p->ptr = NULL;
    // p->ptr_null = TRUE;
    return p;
}

buffer_t *init_buffer_ptr(size_t size) {
    buffer_t *pb = init_buffer_unptr();
    assert(pb != NULL);
    pb->len = 0;
    pb->free = pb->total = size;
    pb->ptr = alloc_buffer(size);
    // pb->ptr_null = FALSE;
    return pb;
}

/**
 * cat the new string after the previous buffer
 * @param buf
 * @param str
 * @param nbytes
 * @return
 */
l_err cat_to_buffer(buffer_t *buf, const uint8_t *str, size_t nbytes) {
    uint8_t *p;
    size_t new_size = buf->len + nbytes;

    /* if previous buffer free space smaller than the new buffer size, need alloc new buffer,
     * the new size is the pre_len + new_len, and the new free space should always be 0. */
    if (nbytes > buf->free) {
        p = alloc_buffer(new_size);
        if (p == NULL) {
            return LD_ERR_INTERNAL;
        }

        memcpy(p, buf->ptr, buf->len);

        free(buf->ptr);
        buf->ptr = p;
        buf->free = 0;
        buf->total = new_size;
        // buf->ptr_null = FALSE;
    } else {
        /* cat directly */
        p = buf->ptr;
        buf->free -= nbytes;
    }

    memcpy(p + buf->len, str, nbytes);
    buf->len = new_size;

    return LD_OK;
}

l_err set_to_buffer(buffer_t *dbuf, int start, uint8_t *src, size_t len) {
    if (dbuf->free - start < len) {
        return LD_ERR_INVALID;
    }
    memcpy(dbuf->ptr + start, src, len);
    dbuf->len += len;
    dbuf->free -= len;
    return LD_OK;
}

uint8_t *clone_to_buffer(const uint8_t *buf, size_t total_len) {
    uint8_t *p = alloc_buffer(total_len);

    if (p == NULL) {
        /* 错误日志， 可参考 exit_log_fun_t */
        return NULL;
    }
    if (buf != NULL) {
        memcpy(p, buf, total_len);
    }
    return p;
}

void buffer_freeptr(buffer_t *pb) {
    if (pb->ptr != NULL) {
        free(pb->ptr);
    }
}

inline void buffer_clear(buffer_t *pb) {
    // log_buf(LOG_INFO, "TO CLEAR", pb->ptr, pb->len);
    // if (pb != NULL && pb->ptr_null == FALSE) {
    if (pb != NULL) {
        pb->len = 0;
        buffer_freeptr(pb);
        pb->ptr = NULL;
        // pb->ptr_null = TRUE;
    }
//    zero(pb);
}

buffer_t *dupl_buffer(buffer_t *old) {
    buffer_t *new_buf = malloc(sizeof(buffer_t));
    CLONE_TO_CHUNK(*new_buf, old->ptr, old->len)
    return new_buf;
}

void free_buffer(void *ptr) {
    buffer_t *buf_p = ptr;
    if (buf_p) {
        buffer_clear(buf_p);
        free(buf_p);
    }
}

l_err change_buffer_len(buffer_t *buf, size_t valid_len) {
    buf->len = buf->total = valid_len;
}
