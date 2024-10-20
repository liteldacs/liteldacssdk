//
// Created by jiaxv on 23-7-30.
//

#ifndef TEST_CLIENT_ALLOC_H
#define TEST_CLIENT_ALLOC_H
#ifdef _Z_MEMORYPOOL_THREAD_
#include <pthread.h>
#endif

#include "../global/ldacs_sim.h"

#define mem_size_t unsigned long long
#define KB (mem_size_t)(1 << 10)
#define MB (mem_size_t)(1 << 20)
#define GB (mem_size_t)(1 << 30)

extern mem_size_t max_mem;
extern mem_size_t mem_pool_size;

typedef struct _mp_chunk {
    mem_size_t alloc_mem;
    struct _mp_chunk *prev, *next;
    int is_free;
} mp_chunk_t;

typedef struct _mp_mem_pool_list {
    char *start;
    unsigned int id;
    mem_size_t mem_pool_size;
    mem_size_t alloc_mem;
    mem_size_t alloc_prog_mem;
    mp_chunk_t *free_list, *alloc_list;
    struct _mp_mem_pool_list *next;
} mp_memory_t;

typedef struct _mp_mem_pool {
    unsigned int last_id;
    int auto_extend;
    mem_size_t mem_pool_size, total_mem_pool_size, max_mem_pool_size;
    struct _mp_mem_pool_list *mlist;
#ifdef _Z_MEMORYPOOL_THREAD_
    pthread_mutex_t lock;
#endif
} mem_poll_t;

extern mem_poll_t mp;
/*
 *  内部工具函数(调试用)
 */

// 所有Memory的数量
void get_memory_list_count(mem_poll_t *mp, mem_size_t *mlist_len);

// 每个Memory的统计信息
void get_memory_info(mem_poll_t *mp,
                     mp_memory_t *mm,
                     mem_size_t *free_list_len,
                     mem_size_t *alloc_list_len);

int get_memory_id(mp_memory_t *mm);

/*
 *  内存池API
 */

int memory_pool_init(mem_size_t maxmempoolsize, mem_size_t mempoolsize, mem_poll_t *mp);

void *memory_pool_alloc(mem_poll_t *mp, mem_size_t wantsize);

int memory_pool_free(mem_poll_t *mp, void *p);

mem_poll_t *memory_pool_clear(mem_poll_t *mp);

int memory_pool_destroy(mem_poll_t *mp);

int memory_pool_set_thread_safe(mem_poll_t *mp, int thread_safe);

/*
 *  内存池信息API
 */

// 总空间
mem_size_t get_total_memory(mem_poll_t *mp);

// 实际分配空间
mem_size_t get_used_memory(mem_poll_t *mp);

float memory_pool_get_usage(mem_poll_t *mp);

// 数据占用空间
mem_size_t get_prog_memory(mem_poll_t *mp);

float memory_pool_get_prog_usage(mem_poll_t *mp);

int test_mem_poll();

void free_malloced_ptr(void *ptr);

#endif //TEST_CLIENT_ALLOC_H
