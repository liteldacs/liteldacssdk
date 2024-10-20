//
// Created by 邹嘉旭 on 2024/1/5.
//
#include <util_core.h>

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

struct a_s {
    buffer_t *b1;
    buffer_t *b2;
    buffer_t *b3;
    buffer_t *b4;
    buffer_t *b5;
};

typedef struct test_pdu_s {
    // buffer_t *sdu_1_6;
    // buffer_t *sdu_7_12;
    // buffer_t *sdu_13_21;
    // buffer_t *sdu_22_27;
    uint16_t slot_ser;
    buffer_t *dc;
} test_pdu_t;

int main() {
    test_pdu_t test_pdus;
    // INIT_BUF_ARRAY_UNPTR(&test_pdus, 4);

    // FREE_BUF_ARRAY_DEEP(&test_pdus, 4);
    INIT_BUF_ARRAY_UNPTR(&test_pdus.dc, 1);
    FREE_BUF_ARRAY_DEEP(&test_pdus.dc, 1);

    log_warn("%p", test_pdus.dc);

    char *str = "ABCD";
    buffer_t *buf = init_buffer_unptr();
    CLONE_TO_CHUNK(*buf, str, strlen(str));
    free_buffer(buf);
    log_warn("%s %p", buf->ptr, buf);
}
