//
// Created by jiaxv on 23-7-2.
//

#ifndef TEST_MSG_BUFFER_H
#define TEST_MSG_BUFFER_H

#define BUFFER_LIMIT (BUFSIZ * 250)

#include "../global/ldacs_sim.h"

typedef struct buffer_s {
    size_t len; /* used space length in buf */
    size_t total;
    size_t free;
    bool ptr_null;
    uint8_t *ptr; /* store data */
} buffer_t;

buffer_t *init_buffer_unptr();

buffer_t *init_buffer_ptr(size_t size);

static inline size_t buffer_avail(const buffer_t *pb) { return pb->free; }
static inline size_t buffer_len(const buffer_t *pb) { return pb->len; }

extern void buffer_clear(buffer_t *pb);

static inline uint8_t *buffer_end(const buffer_t *pb) {
    return pb->ptr + pb->len;
}

l_err cat_to_buffer(buffer_t *buf, const uint8_t *str, size_t nbytes);

l_err set_to_buffer(buffer_t *dbuf, int start, uint8_t *src, size_t len);

uint8_t *clone_to_buffer(const uint8_t *buf, size_t total_len);

buffer_t *dupl_buffer(buffer_t *old);

void free_buffer(void *ptr);

l_err change_buffer_len(buffer_t *buf, size_t valid_len);


//TODO: 如果ch之前ptr指针不为空，则会造成内存泄漏，需解决
//按长度克隆
//不clear掉之前的buffer则会导致内存泄漏
#define CLONE_TO_CHUNK(ch, src, size)                                     \
    {                                                                   \
        buffer_clear(&(ch)); \
        (ch).ptr = clone_to_buffer((src), (size));                          \
        (ch).free = 0;                                                  \
        (ch).len  = (size);                                               \
        (ch).total  = (size);\
        (ch).ptr_null = FALSE;\
    }

//有最大值的按长度克隆
#define CLONE_TO_CHUNK_L(ch, src, size, total_len)                         \
    {                                                                      \
        (ch).ptr = clone_to_buffer((src), (total_len));                     \
        (ch).len = (size);                                                \
        (ch).free = (total_len) - (size);                                   \
        (ch).total = (total_len);                                         \
        (ch).ptr_null = FALSE;\
    }

//根据静态buffer克隆（按长度）
#define CLONE_BY_BUFFER(ch, buf) \
    {                              \
        CLONE_TO_CHUNK((ch), (buf).ptr, (buf).len); \
        buffer_clear(&(buf));                           \
    }

//根据静态buffer克隆（按长度）
#define CLONE_BY_BUFFER_UNFREE(ch, buf) \
    {                              \
        CLONE_TO_CHUNK((ch), (buf).ptr, (buf).len); \
    }

//根据malloc过的buffer克隆（按长度）
//#define CLONE_BY_ALLOCED_BUFFER(ch, tmp) \
//{                                 \
//    CLONE_BY_BUFFER((ch), (tmp));        \
//    free(&(tmp));\
//}

//根据malloc过的buffer克隆（按长度）
static void clone_by_alloced_buffer(buffer_t *buf, buffer_t *src) {
    CLONE_TO_CHUNK(*buf, src->ptr, src->len);
    //free_buffer_v(&src);
    free_buffer(src);
}


#define clonebybufferl(ch, buf, total_len) \
    {                              \
        CLONE_TO_CHUNK_L(ch, (buf).ptr, (buf).len, total_len); \
        buffer_clear(&buf);                           \
    }

//初始化buffer数组
#define INIT_BUF_ARRAY_UNPTR(buf_p, n) \
{                                      \
    /* void *ptr = (void *)(buf_p); */  \
    buffer_t **ptr = (buffer_t **)(buf_p);  \
    for(int i = 0; i < (n); i++){                   \
        /*(*(buffer_t **)(ptr + i * sizeof(void *))) = init_buffer_unptr(); */       \
        ptr[i] = init_buffer_unptr(); \
    }                                               \
}

//free掉malloc过的数组
#define FREE_BUF_ARRAY_DEEP(buf_p, n) \
{                                      \
    buffer_t **ptr = (buffer_t **)(buf_p); \
    for(int i = 0; i < (n); i++){                   \
        free_buffer(ptr[i]);\
    }                                               \
    memset(buf_p, 0x00, n);\
}


//#define INIT_BUF_ARRAY_UNPTR(buf_p, n) \
//{                                      \
//    for(int i = 0; i < (n); i++){                   \
//        (buf_p )[i] = malloc(sizeof(buffer_t));        \
//    }                                               \
//}

#define INIT_BUF_ARRAY_PTR(buf_p, n, size) \
{ \
    for(int i = 0; i < (n); i++){                   \
        (buf_p)[i] = init_buffer(size);        \
    }                                               \
}

//只free掉数据，不free数组
#define FREE_BUF_ARRAY(buf_p, n) \
{                                \
    for(int i = 0; i < (n); i++){\
        buffer_clear((buf_p)[i]);                             \
    }                                               \
}


#endif //TEST_MSG_BUFFER_H
