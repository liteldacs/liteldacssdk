//
// Created by 邹嘉旭 on 2024/10/8.
//

#include "../global/ldacs_sim.h"
#include "ld_rbuffer.h"

typedef struct test_c {
    uint32_t a;
    char b;
} test_t;

bool test_factor(const void *a, const void *b) {
    if (*((int *) a) == (*(int *) b)) return TRUE;
    return FALSE;
}

int main() {
    ld_rbuffer *rb = ld_rbuffer_init(3);
    int a = 1, b = 2, c = 3;
    ld_rbuffer_push_back(rb, &a); // 插入尾部
    ld_rbuffer_push_front(rb, &b); // 插入头部
    ld_rbuffer_push_back(rb, &c); // 插入尾部
    int *f;
    ld_rbuffer_get_front(rb, &f);
    printf("Front element: %d\n", *f);
    int *pop_data;
    ld_rbuffer_pop(rb, &pop_data);
    printf("Popped element: %d\n", *pop_data);
    ld_rbuffer_pop(rb, &pop_data);
    printf("Popped element: %d\n", *pop_data);
    ld_rbuffer_pop(rb, &pop_data);
    printf("Popped element: %d\n", *pop_data);

    ld_rbuffer_free(rb);


    return 0;
}

