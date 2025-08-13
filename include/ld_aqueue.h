//
// Created by jiaxv on 25-8-14.
//

#ifndef LD_AQUEUE_H
#define LD_AQUEUE_H
/* 2015 Daniel Bittman <danielbittman1@gmail.com>: http://dbittman.github.io/ */
#ifndef __MPSCQ_H
#define __MPSCQ_H

#include <stdint.h>
#ifndef __cplusplus
#include <stdatomic.h>
#endif
#include <stdbool.h>
#include <sys/types.h>

#define MPSCQ_MALLOC 1

#ifndef __cplusplus
typedef struct ld_aqueue_s {
    _Atomic size_t count;
    _Atomic size_t head;
    size_t tail;
    size_t max;
    void * _Atomic *buffer;
    int flags;
}ld_aqueue_t;
#else
struct mpscq;
#endif

#ifdef __cplusplus
extern "C" {
#endif
    /* create a new mpscq. If n == NULL, it will allocate
     * a new one and return it. If n != NULL, it will
     * initialize the structure that was passed in.
     * capacity must be greater than 1, and it is recommended
     * to be much, much larger than that. It must also be a power of 2. */
    ld_aqueue_t *ld_aqueue_create(ld_aqueue_t *n, size_t capacity);

    /* enqueue an item into the queue. Returns true on success
     * and false on failure (queue full). This is safe to call
     * from multiple threads */
    bool ld_aqueue_enqueue(ld_aqueue_t *q, void *obj);

    /* dequeue an item from the queue and return it.
     * THIS IS NOT SAFE TO CALL FROM MULTIPLE THREADS.
     * Returns NULL on failure, and the item it dequeued
     * on success */
    void *ld_aqueue_dequeue(ld_aqueue_t *q);

    /* get the number of items in the queue currently */
    size_t ld_aqueue_count(ld_aqueue_t *q);

    /* get the capacity of the queue */
    size_t ld_aqueue_capacity(ld_aqueue_t *q);

    /* destroy a mpscq. Frees the internal buffer, and
     * frees q if it was created by passing NULL to mpscq_create */
    void ld_aqueue_destory(ld_aqueue_t *q);

#ifdef __cplusplus
}
#endif

#endif

#endif //LD_AQUEUE_H
