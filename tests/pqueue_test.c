/*
 * Copyright (c) 2014, Volkan Yazıcı <volkan.yazici@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * A simple usage of libpqueue. (Priorities are represented as "double" values.)
 */

#include <ld_pqueue.h>

typedef struct pq_element_s {
    pqueue_pri_t pri;
    size_t pos;
    void *val;
} pq_element_t;

static int default_cmp_pri(pqueue_pri_t next, pqueue_pri_t curr) {
    return (next >= curr);
}

static pqueue_pri_t default_get_pri(void *a) {
    return ((pq_element_t *) a)->pri;
}


static void default_set_pri(void *a, pqueue_pri_t pri) {
    ((pq_element_t *) a)->pri = pri;
}


static size_t default_get_pos(void *a) {
    return ((pq_element_t *) a)->pos;
}


static void default_set_pos(void *a, size_t pos) {
    ((pq_element_t *) a)->pos = pos;
}


static void print_func(FILE *out, void *a) {
    pq_element_t *n = a;
    fprintf(out, "%d\n", *(int *) n->val);
}

//
// int main() {
//     pqueue_t *pq;
//     pqnode_t *ns;
//     pqnode_t *n;
//
//     ns = malloc(10 * sizeof(pqnode_t));
//     pq = pqueue_init(10, default_cmp_pri, default_get_pri, default_set_pri, default_get_pos, default_set_pos);
//     if (!(ns && pq)) return 1;
//
//     int val_0 = -5, val_1 = -4, val_2 = -2, val_3 = -6, val_4 = -1, val_5 = -77;
//     log_warn("%d", pqueue_size(pq)) ;
//     ns[0].pri = 5;
//     ns[0].val = -5;
//     pqueue_insert(pq, &ns[0]);
//     ns[1].pri = 4;
//     ns[1].val = -4;
//     pqueue_insert(pq, &ns[1]);
//     ns[2].pri = 2;
//     ns[2].val = -2;
//     pqueue_insert(pq, &ns[2]);
//     ns[3].pri = 6;
//     ns[3].val = -6;
//     pqueue_insert(pq, &ns[3]);
//     ns[4].pri = 1;
//     ns[4].val = -1;
//     pqueue_insert(pq, &ns[4]);
//     ns[5].pri = 4;
//     ns[1].val = -77;
//     pqueue_insert(pq, &ns[5]);
//
//     pqueue_dump(pq, stdout, print_func);
//
//
//     pqueue_peek(pq, (void **) &n);
//     printf("peek: %lld [%d]\n", n->pri, n->val);
//
//     pqueue_change_priority(pq, 8, &ns[4]);
//     pqueue_change_priority(pq, 7, &ns[2]);
//
//     pqueue_pop(pq, (void **) &n);
//     while (n) {
//         printf("pop: %lld [%d]\n", n->pri, n->val);
//         pqueue_pop_wait(pq, (void **) &n);
//     }
//
//     pqueue_free(pq);
//     free(ns);
//
//     return 0;
// }

int main() {
    pqueue_t *pq;
    pq_element_t *ns;
    pq_element_t *n;

    ns = malloc(10 * sizeof(pq_element_t));
    pq = pqueue_init(10, default_cmp_pri, default_get_pri, default_set_pri, default_get_pos, default_set_pos);
    if (!(ns && pq)) return 1;

    int val_0 = -5, val_1 = -4, val_2 = -2, val_3 = -6, val_4 = -1, val_5 = -77;
    log_warn("%d", pqueue_size(pq));
    ns[0].val = &val_0;
    pqueue_insert(pq, &ns[0]);
    ns[1].pri = 4;
    ns[1].val = &val_1;
    pqueue_insert(pq, &ns[1]);
    ns[2].pri = 2;
    ns[2].val = &val_2;
    pqueue_insert(pq, &ns[2]);
    ns[3].pri = 6;
    ns[3].val = &val_3;
    pqueue_insert(pq, &ns[3]);
    ns[4].pri = 1;
    ns[4].val = &val_4;
    pqueue_insert(pq, &ns[4]);
    ns[5].pri = 4;
    ns[5].val = &val_5;
    pqueue_insert(pq, &ns[5]);

    pqueue_dump(pq, stdout, print_func);

    pqueue_peek(pq, (void **) &n);
    printf("peek: %lld [%d]\n", n->pri, *(int *) n->val);

    pqueue_change_priority(pq, 8, &ns[4]);
    pqueue_change_priority(pq, 7, &ns[2]);

    pqueue_flush(pq, NULL);
    log_warn("%d", pqueue_size(pq));
    pq_element_t *n2 = NULL;
    pqueue_pop(pq, (void **) &n2);
    if (n2 == NULL) {
        log_error("!!!!!");
    }


    //pqueue_pop(pq, (void **) &n);
    while (n) {
        pqueue_pop_wait(pq, (void **) &n);
        printf("pop: %lld [%d]\n", n->pri, *(int *) n->val);
    }

    pqueue_free(pq);
    free(ns);

    return 0;
}
