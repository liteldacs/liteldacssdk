//
// Created by 邹嘉旭 on 2024/1/5.
//
#include <ld_buffer.h>
#include <ld_log.h>

static uint8_t u1[3] = {0x01, 0x03, 0x05};
static uint8_t u2[3] = {0x01, 0x03, 0x05};

void test_normal_cat() {
    buffer_t buf1;
    CLONE_TO_CHUNK(buf1, u1, 3);
    cat_to_buffer(&buf1, u2, 3);

    for (int i = 0; i < 7; i++) {
        fprintf(stderr, "%d ", buf1.ptr[i]);
    }
    fprintf(stderr, "\nREMAIN: %zu\n", buf1.free);
}

void test_empty_cat() {
    buffer_t *buf = init_buffer_ptr(9);
    cat_to_buffer(buf, u2, 3);
    cat_to_buffer(buf, u2, 3);
    cat_to_buffer(buf, u2, 3);

    for (int i = 0; i < 9; i++) {
        fprintf(stderr, "%d ", buf->ptr[i]);
    }
    fprintf(stderr, "\nREMAIN: %zu\n", buf->free);
}

typedef struct sdu_s_s {
    size_t blk_n;
    buffer_t **blks;
} sdu_s_t;

static inline sdu_s_t *create_sdu_s(size_t blk_n) {
    sdu_s_t *sdu_s_p = malloc(sizeof(sdu_s_t));
    sdu_s_p->blk_n = blk_n;
    sdu_s_p->blks = calloc(sdu_s_p->blk_n, sizeof(buffer_t *));
    return sdu_s_p;
}
static inline void free_sdu_s(void *p) {
    sdu_s_t *sdu_p = p;
    if (sdu_p) {
        if (sdu_p->blks) {
            FREE_BUF_ARRAY_DEEP2(sdu_p->blks, sdu_p->blk_n);
        }
        free(sdu_p);
    }
}

#define BC_BLK_N 3
#define BC_BLK_LEN_1_3  528 >> 3  //66 Bytes
#define BC_BLK_LEN_2    1000 >> 3 //125 Bytes
#define BC_BLK_FORCE_REMAIN 272 >> 8 // 272 = 4 + 256 + 4 + 8

int main() {
    sdu_s_t *sdus = create_sdu_s(BC_BLK_N);
    sdus->blks[0] = init_buffer_ptr(BC_BLK_LEN_1_3);
    sdus->blks[1] = init_buffer_ptr(BC_BLK_LEN_2);
    sdus->blks[2] = init_buffer_ptr(BC_BLK_LEN_1_3);


    free_sdu_s(sdus);
}
