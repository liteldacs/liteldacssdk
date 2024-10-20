#include "ld_alloc.h"


mem_size_t max_mem = 2 * GB + 1000 * MB + 1000 * KB;
mem_size_t mem_pool_size = 1 * GB + 500 * MB + 500 * KB;

mem_poll_t mp;
#define MP_CHUNKHEADER sizeof(struct _mp_chunk)
#define MP_CHUNKEND sizeof(struct _mp_chunk*)

#define MP_LOCK(lockobj)                    \
    do {                                    \
        pthread_mutex_lock(&lockobj->lock); \
    } while (0)
#define MP_UNLOCK(lockobj)                    \
    do {                                      \
        pthread_mutex_unlock(&lockobj->lock); \
    } while (0)

#define MP_ALIGN_SIZE(_n) (_n + sizeof(long) - ((sizeof(long) - 1) & _n))

#define MP_INIT_MEMORY_STRUCT(mm, mem_sz)       \
    do {                                        \
        mm->mem_pool_size = mem_sz;             \
        mm->alloc_mem = 0;                      \
        mm->alloc_prog_mem = 0;                 \
        mm->free_list = (mp_chunk_t*) mm->start; \
        mm->free_list->is_free = 1;             \
        mm->free_list->alloc_mem = mem_sz;      \
        mm->free_list->prev = NULL;             \
        mm->free_list->next = NULL;             \
        mm->alloc_list = NULL;                  \
    } while (0)

#define MP_DLINKLIST_INS_FRT(head, x) \
    do {                              \
        x->prev = NULL;               \
        x->next = head;               \
        if (head) head->prev = x;     \
        head = x;                     \
    } while (0)

#define MP_DLINKLIST_DEL(head, x)                 \
    do {                                          \
        if (!x->prev) {                           \
            head = x->next;                       \
            if (x->next) x->next->prev = NULL;    \
        } else {                                  \
            x->prev->next = x->next;              \
            if (x->next) x->next->prev = x->prev; \
        }                                         \
    } while (0)

void get_memory_list_count(mem_poll_t *mp, mem_size_t *mlist_len) {
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_LOCK(mp);
#endif
    mem_size_t mlist_l = 0;
    mp_memory_t *mm = mp->mlist;
    while (mm) {
        mlist_l++;
        mm = mm->next;
    }
    *mlist_len = mlist_l;
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_UNLOCK(mp);
#endif
}

void get_memory_info(mem_poll_t *mp,
                     mp_memory_t *mm,
                     mem_size_t *free_list_len,
                     mem_size_t *alloc_list_len) {
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_LOCK(mp);
#endif
    mem_size_t free_l = 0, alloc_l = 0;
    mp_chunk_t *p = mm->free_list;
    while (p) {
        free_l++;
        p = p->next;
    }

    p = mm->alloc_list;
    while (p) {
        alloc_l++;
        p = p->next;
    }
    *free_list_len = free_l;
    *alloc_list_len = alloc_l;
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_UNLOCK(mp);
#endif
}

int get_memory_id(mp_memory_t *mm) {
    return mm->id;
}

static mp_memory_t *extend_memory_list(mem_poll_t *mp, mem_size_t new_mem_sz) {
    char *s = (char *) malloc(sizeof(mp_memory_t) + new_mem_sz * sizeof(char));
    if (!s) return NULL;

    mp_memory_t *mm = (mp_memory_t *) s;
    mm->start = s + sizeof(mp_memory_t);

    MP_INIT_MEMORY_STRUCT(mm, new_mem_sz);
    mm->id = mp->last_id++;
    mm->next = mp->mlist;
    mp->mlist = mm;
    return mm;
}

static mp_memory_t *find_memory_list(mem_poll_t *mp, void *p) {
    mp_memory_t *tmp = mp->mlist;
    while (tmp) {
        if (tmp->start <= (char *) p &&
            tmp->start + mp->mem_pool_size > (char *) p)
            break;
        tmp = tmp->next;
    }

    return tmp;
}

static int merge_free_chunk(mem_poll_t *mp, mp_memory_t *mm, mp_chunk_t *c) {
    mp_chunk_t *p0 = c, *p1 = c;
    while (p0->is_free) {
        p1 = p0;
        if ((char *) p0 - MP_CHUNKEND - MP_CHUNKHEADER <= mm->start) break;
        p0 = *(mp_chunk_t **) ((char *) p0 - MP_CHUNKEND);
    }

    p0 = (mp_chunk_t *) ((char *) p1 + p1->alloc_mem);
    while ((char *) p0 < mm->start + mp->mem_pool_size && p0->is_free) {
        MP_DLINKLIST_DEL(mm->free_list, p0);
        p1->alloc_mem += p0->alloc_mem;
        p0 = (mp_chunk_t *) ((char *) p0 + p0->alloc_mem);
    }

    *(mp_chunk_t **) ((char *) p1 + p1->alloc_mem - MP_CHUNKEND) = p1;
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_UNLOCK(mp);
#endif
    return 0;
}

int memory_pool_init(mem_size_t maxmempoolsize, mem_size_t mempoolsize, mem_poll_t * mp) {

    do {
        if (mempoolsize > maxmempoolsize) break;

        //mem_poll_t *mp = (mem_poll_t *) malloc(sizeof(mem_poll_t));
        //mp = (mem_poll_t *) malloc(sizeof(mem_poll_t));
        if (!mp) break;

        mp->last_id = 0;
        if (mempoolsize < maxmempoolsize) mp->auto_extend = 1;
        mp->max_mem_pool_size = maxmempoolsize;
        mp->total_mem_pool_size = mp->mem_pool_size = mempoolsize;

#ifdef _Z_MEMORYPOOL_THREAD_
        pthread_mutex_init(&mp->lock, NULL);
#endif

        char *s = (char *) malloc(sizeof(mp_memory_t) +
                                  sizeof(char) * mp->mem_pool_size);
        if (!s) break;

        mp->mlist = (mp_memory_t *) s;
        mp->mlist->start = s + sizeof(mp_memory_t);
        MP_INIT_MEMORY_STRUCT(mp->mlist, mp->mem_pool_size);
        mp->mlist->next = NULL;
        mp->mlist->id = mp->last_id++;
        return TRUE;
    } while (0);


    return FALSE;
}

void *memory_pool_alloc(mem_poll_t *mp, mem_size_t wantsize) {
    if (wantsize <= 0) return NULL;
    mem_size_t total_needed_size =
            MP_ALIGN_SIZE(wantsize + MP_CHUNKHEADER + MP_CHUNKEND);
    if (total_needed_size > mp->mem_pool_size) return NULL;

    mp_memory_t *mm = NULL, *mm1 = NULL;
    mp_chunk_t *_free = NULL, *_not_free = NULL;
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_LOCK(mp);
#endif
    FIND_FREE_CHUNK:
    mm = mp->mlist;
    while (mm) {
        if (mp->mem_pool_size - mm->alloc_mem < total_needed_size) {
            mm = mm->next;
            continue;
        }

        _free = mm->free_list;
        _not_free = NULL;

        while (_free) {
            if (_free->alloc_mem >= total_needed_size) {
                // 如果free块分割后剩余内存足够大 则进行分割
                if (_free->alloc_mem - total_needed_size >
                    MP_CHUNKHEADER + MP_CHUNKEND) {
                    // 从free块头开始分割出alloc块
                    _not_free = _free;

                    _free = (mp_chunk_t *) ((char *) _not_free +
                                            total_needed_size);
                    *_free = *_not_free;
                    _free->alloc_mem -= total_needed_size;
                    *(mp_chunk_t **) ((char *) _free + _free->alloc_mem -
                                      MP_CHUNKEND) = _free;

                    // update free_list
                    if (!_free->prev) {
                        mm->free_list = _free;
                    } else {
                        _free->prev->next = _free;
                    }
                    if (_free->next) _free->next->prev = _free;

                    _not_free->is_free = 0;
                    _not_free->alloc_mem = total_needed_size;

                    *(mp_chunk_t **) ((char *) _not_free + total_needed_size -
                                      MP_CHUNKEND) = _not_free;
                }
                    // 不够 则整块分配为alloc
                else {
                    _not_free = _free;
                    MP_DLINKLIST_DEL(mm->free_list, _not_free);
                    _not_free->is_free = 0;
                }
                MP_DLINKLIST_INS_FRT(mm->alloc_list, _not_free);

                mm->alloc_mem += _not_free->alloc_mem;
                mm->alloc_prog_mem +=
                        (_not_free->alloc_mem - MP_CHUNKHEADER - MP_CHUNKEND);
#ifdef _Z_MEMORYPOOL_THREAD_
                MP_UNLOCK(mp);
#endif
                return (void *) ((char *) _not_free + MP_CHUNKHEADER);
            }
            _free = _free->next;
        }

        mm = mm->next;
    }

    if (mp->auto_extend) {
        // 超过总内存限制
        if (mp->total_mem_pool_size >= mp->max_mem_pool_size) {
            goto err_out;
        }
        mem_size_t add_mem_sz = mp->max_mem_pool_size - mp->mem_pool_size;
        add_mem_sz = add_mem_sz >= mp->mem_pool_size ? mp->mem_pool_size
                                                     : add_mem_sz;
        mm1 = extend_memory_list(mp, add_mem_sz);
        if (!mm1) {
            goto err_out;
        }
        mp->total_mem_pool_size += add_mem_sz;

        goto FIND_FREE_CHUNK;
    }

    err_out:
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_UNLOCK(mp);
#endif
    return NULL;
}

int memory_pool_free(mem_poll_t *mp, void *p) {
    if (p == NULL || mp == NULL) return 1;
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_LOCK(mp);
#endif
    mp_memory_t *mm = mp->mlist;
    if (mp->auto_extend) mm = find_memory_list(mp, p);

    mp_chunk_t *ck = (mp_chunk_t *) ((char *) p - MP_CHUNKHEADER);

    MP_DLINKLIST_DEL(mm->alloc_list, ck);
    MP_DLINKLIST_INS_FRT(mm->free_list, ck);
    ck->is_free = 1;

    mm->alloc_mem -= ck->alloc_mem;
    mm->alloc_prog_mem -= (ck->alloc_mem - MP_CHUNKHEADER - MP_CHUNKEND);

    return merge_free_chunk(mp, mm, ck);
}

mem_poll_t *memory_pool_clear(mem_poll_t *mp) {
    if (!mp) return NULL;
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_LOCK(mp);
#endif
    mp_memory_t *mm = mp->mlist;
    while (mm) {
        MP_INIT_MEMORY_STRUCT(mm, mm->mem_pool_size);
        mm = mm->next;
    }
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_UNLOCK(mp);
#endif
    return mp;
}

int memory_pool_destroy(mem_poll_t *mp) {
    if (mp == NULL) return 1;

#ifdef _Z_MEMORYPOOL_THREAD_
    MP_LOCK(mp);
#endif
    mp_memory_t *mm = mp->mlist, *mm1 = NULL;
    while (mm) {
        mm1 = mm;
        mm = mm->next;
        free(mm1);
    }
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_UNLOCK(mp);
    pthread_mutex_destroy(&mp->lock);
#endif
    fprintf(stderr, "%p\n", mp);
    free(mp);
    return 0;
}

mem_size_t get_total_memory(mem_poll_t *mp) {
    return mp->total_mem_pool_size;
}

mem_size_t get_used_memory(mem_poll_t *mp) {
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_LOCK(mp);
#endif
    mem_size_t total_alloc = 0;
    mp_memory_t *mm = mp->mlist;
    while (mm) {
        total_alloc += mm->alloc_mem;
        mm = mm->next;
    }
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_UNLOCK(mp);
#endif
    return total_alloc;
}

mem_size_t get_prog_memory(mem_poll_t *mp) {
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_LOCK(mp);
#endif
    mem_size_t total_alloc_prog = 0;
    mp_memory_t *mm = mp->mlist;
    while (mm) {
        total_alloc_prog += mm->alloc_prog_mem;
        mm = mm->next;
    }
#ifdef _Z_MEMORYPOOL_THREAD_
    MP_UNLOCK(mp);
#endif
    return total_alloc_prog;
}

float memory_pool_get_usage(mem_poll_t *mp) {
    return (float) get_used_memory(mp) / get_total_memory(mp);
}

float memory_pool_get_prog_usage(mem_poll_t *mp) {
    return (float) get_prog_memory(mp) / get_total_memory(mp);
}

struct TAT {
    int T_T;
};

/*int test_mem_poll() {
    memory_pool_init(max_mem, mem_pool_size, &mp);
    struct TAT* tat = (struct TAT*) memory_pool_alloc(&mp, sizeof(struct TAT));
    tat->T_T = 2333;
    printf("%d\n", tat->T_T);
    memory_pool_free(&mp, tat);
    memory_pool_clear(&mp);
    //memory_pool_destroy(&mp);
    return 0;
}*/

void free_malloced_ptr(void *ptr){
    if(ptr){
        free(ptr);
    }
}

#undef MP_CHUNKHEADER
#undef MP_CHUNKEND
#undef MP_LOCK
#undef MP_ALIGN_SIZE
#undef MP_INIT_MEMORY_STRUCT
#undef MP_DLINKLIST_INS_FRT
#undef MP_DLINKLIST_DEL