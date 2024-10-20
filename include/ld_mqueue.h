//
// Created by 邹嘉旭 on 2024/7/19.
//

#ifndef LD_MQUEUE_H
#define LD_MQUEUE_H

#include "../global/ldacs_sim.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lfqueue_cas_node_s lfqueue_cas_node_t;

typedef void * (*lfqueue_malloc_fn)(void *, size_t);

typedef void (*lfqueue_free_fn)(void *, void *);

#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
#define lfq_bool_t int
#else
#ifdef _WIN64
#define lfq_bool_t int64_t
#else
#define lfq_bool_t int
#endif
#endif

typedef struct {
    lfqueue_cas_node_t *head, *tail, *root_free, *move_free;
    volatile size_t size;
    volatile lfq_bool_t in_free_mode;
    lfqueue_malloc_fn _malloc;
    lfqueue_free_fn _free;
    void *pl;
} lfqueue_t;

extern lfqueue_t *lfqueue_init();

extern lfqueue_t *lfqueue_init_mf(void *pl, lfqueue_malloc_fn lfqueue_malloc,
                                  lfqueue_free_fn lfqueue_free);

extern l_err lfqueue_put(lfqueue_t *lfqueue, void *value);

extern void *lfqueue_deq(lfqueue_t *lfqueue);

l_err lfqueue_get(lfqueue_t *, void **);

l_err lfqueue_get_wait(lfqueue_t *lfqueue, void **v);


extern void *lfqueue_single_deq(lfqueue_t *lfqueue);

/** loop until value been dequeue, it sleeps 1ms if not found to reduce cpu high usage **/
extern void *lfqueue_wait_while(void *lfqueue);

extern void *lfqueue_single_deq_must(lfqueue_t *lfqueue);

extern void lfqueue_destroy(lfqueue_t *lfqueue);

extern void lfqueue_free(lfqueue_t *queue);

extern size_t lfqueue_size(lfqueue_t *lfqueue);

extern void lfqueue_sleep(unsigned int milisec);


#ifdef __cplusplus
}
#endif
#endif //LD_MQUEUE_H
