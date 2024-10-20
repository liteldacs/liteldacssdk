//
// Created by 邹嘉旭 on 2024/3/26.
//

#include <ld_log.h>

#include "ld_mqueue.h"
#include "../global/ldacs_sim.h"

typedef struct node {
    int i;
    double d;
} node;

pthread_t th;

void *func(void *arg) {
    lfqueue_t *queue = arg;
    int i = 5;
    while (i--) {
        node *nd = malloc(sizeof(node));
        nd->i = i;
        nd->d = 666.66;

        lfqueue_put(queue, nd);
        log_warn("%d", lfqueue_size(queue));
        sleep(2);
    }
    return NULL;
}

int main() {
    lfqueue_t *queue = lfqueue_init();

    pthread_create(&th, NULL, func, queue);

    while (1) {
        node *nd_2;

        lfqueue_get_wait(queue, (void **) &nd_2);
        log_warn("%.2f", nd_2->d);
        free(nd_2);
    }

    lfqueue_destroy(queue);
    lfqueue_free(queue);
}
