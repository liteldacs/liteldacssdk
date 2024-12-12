#include <stdio.h>
#include "ld_alloc.h"

struct TAT {
    int T_T;
};

mem_size_t max_mem = 2 * GB + 1000 * MB + 1000 * KB;
mem_size_t mempool_size = 1 * GB + 500 * MB + 500 * KB;

int main() {
    ld_mempool* mp = mempool_init(max_mem, mempool_size);
    struct TAT* tat = (struct TAT*) mempool_alloc(mp, sizeof(struct TAT));
    tat->T_T = 2333;
    printf("%d\n", tat->T_T);
    mempool_free(mp, tat);
    mempool_clear(mp);
    mempool_destroy(mp);
    return 0;
}