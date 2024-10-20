//
// Created by jiaxv on 23-9-27.
//

#ifndef LDACS_SIM_LD_HEAP_H
#define LDACS_SIM_LD_HEAP_H

#include "../global/ldacs_sim.h"
#include "ld_util_def.h"

#define MAX_HEAP (8192)
#define SADDR_STG_SIZE sizeof(struct sockaddr_storage)
#define LCHILD(x) (((x) << 1) + 1)
#define RCHILD(x) (LCHILD(x) + 1)
#define PARENT(x) ((x - 1) >> 1)
#define INHEAP(n, x) (((-1) < (x)) && ((x) < (n)))

typedef struct heap_s {
    int heap_idx; /* idx at lotos_connections */
    int64_t factor;
    void *obj;
} heap_t;

typedef struct heap_desc_s {
    heap_t *hps[MAX_HEAP];
    int heap_size;
} heap_desc_t;


void init_heap_desc(heap_desc_t *hd);

void heap_bubble_down(heap_desc_t *hd, int idx);

int heap_insert(heap_desc_t *hd, void *obj, int64_t factor);

void heap_bubble_up(heap_desc_t *hd, int idx);

heap_t *get_heap(heap_desc_t *hd, void *obj_p);

#endif //LDACS_SIM_LD_HEAP_H
