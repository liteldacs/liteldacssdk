//
// Created by jiaxv on 23-9-27.
//

#include "ld_heap.h"

void init_heap_desc(heap_desc_t *hd){
    zero(&hd->hps);
    hd->heap_size = 0;
}

inline static void c_swap(heap_desc_t *hd, int x, int y) {
    assert(x >= 0 && x < hd->heap_size && y >= 0 && y < hd->heap_size);
    heap_t *tmp = hd->hps[x];
    hd->hps[x] = hd->hps[y];
    hd->hps[y] = tmp;
    // update heap_idx
    hd->hps[x]->heap_idx = x;
    hd->hps[y]->heap_idx = y;
}

void heap_bubble_up(heap_desc_t *hd, int idx) {
    while (PARENT(idx) >= 0) {
        int fidx = PARENT(idx); // fidx is father of idx;
        heap_t *c = hd->hps[idx];
        heap_t *fc = hd->hps[fidx];
        if (c->factor >= fc->factor)
            break;
        c_swap(hd, idx, fidx);
        idx = fidx;
    }
}


int heap_insert(heap_desc_t *hd, void *obj, int64_t factor) {
    heap_t *heap_node = (heap_t *) malloc(sizeof (heap_t));
    heap_node->obj = obj;
    heap_node->factor = factor;
    heap_node->heap_idx = hd->heap_size;
    hd->hps[hd->heap_size++] = heap_node;

    heap_bubble_up(hd, hd->heap_size - 1);
    return OK;
}

/* used for extracting or active_time update larger */
void heap_bubble_down(heap_desc_t *hd, int idx) {
    while (TRUE) {
        int proper_child;
        int lchild = INHEAP(hd->heap_size, LCHILD(idx)) ? LCHILD(idx) : (hd->heap_size + 1);
        int rchild = INHEAP(hd->heap_size, RCHILD(idx)) ? RCHILD(idx) : (hd->heap_size + 1);
        if (lchild > hd->heap_size && rchild > hd->heap_size) { // no children
            break;
        } else if (INHEAP(hd->heap_size, lchild) && INHEAP(hd->heap_size, rchild)) {
            proper_child = hd->hps[lchild]->factor <
                           hd->hps[rchild]->factor
                           ? lchild
                           : rchild;
        } else if (lchild > hd->heap_size) {
            proper_child = rchild;
        } else {
            proper_child = lchild;
        }
        // idx is the smaller than children
        if (hd->hps[idx]->factor <=
            hd->hps[proper_child]->factor)
            break;
        assert(INHEAP(hd->heap_size, proper_child));
        c_swap(hd, idx, proper_child);
        idx = proper_child;
    }
}

heap_t *get_heap(heap_desc_t*hd, void *obj_p){
    for(int i = 0; i < hd->heap_size; i++){
        if(hd->hps[i]->obj == obj_p){
            return hd->hps[i];
        }
    }
    return NULL;
}