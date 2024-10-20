//
// Created by 邹嘉旭 on 2024/1/7.
//

#include "msg_core.h"

#pragma pack(1)
struct test_pdu{
    uint32_t type;
    buffer_t *buf;
    buffer_t *buf2;
    uint32_t type2;
    uint16_t plen;
    buffer_t *sdu;
};
#pragma pack()

static const size_t temp_size = 8;
static field_desc test_pdu_fields[] = {
        {ft_set, 29, "TYPE", NULL},
        {ft_dl_str, 0, "STR", NULL},
        {ft_dl_str, 0, "STR", NULL},
        {ft_set, 30, "TYPE", NULL},
        {ft_plen, 10, "PLEN", NULL},
        {ft_pad, 0, "Test_pad", NULL},
        {ft_p_raw, 0, "SDU", &temp_size},
        {ft_crc, CRC_8_SIZE, "crc", NULL},
        {ft_end, 0, NULL,  NULL},
};

struct_desc_t static test_pdu_desc = {"SNP_PDU_HEAD", test_pdu_fields};

#define SIZE 4
int main(){
    uint8_t test_str[SIZE] = {0xe2, 0xd4, 0x16, 0x58};
    uint8_t cs[512] = {0};
    buffer_t buf;
    pb_stream pbs;
    pb_stream pbs_i;
    CLONE_TO_CHUNK(buf, test_str, SIZE)
    struct test_pdu tp = {
            .type = 114514,
            .buf = &buf,
            .buf2 = &buf,
            .type2 = 0x172122,
            .sdu = &buf,
    };

    struct test_pdu *tp_i = malloc(sizeof(struct test_pdu));
    tp_i->buf = init_buffer_ptr(SIZE);
    tp_i->buf2 = init_buffer_ptr(SIZE);
    tp_i->sdu = init_buffer_unptr();

    init_pbs(&pbs, cs, 512, "Test");
    out_struct(&tp, &test_pdu_desc, &pbs, NULL);
    close_output_pbs(&pbs);
    //log_print_buffer(LOG_INFO, "Test", pbs.start, pbs_offset(&pbs));
    log_buf(LOG_INFO, "Test", pbs.start, pbs_offset(&pbs));


    init_pbs(&pbs_i, pbs.start, pbs_offset(&pbs), "Test");
    in_struct(tp_i, &test_pdu_desc, &pbs_i, NULL);


    fprintf(stderr, "TYPE1:  %d\n", tp_i->type);
    fprintf(stderr, "TYPE2:  %d\n", tp_i->type2);
    //fprintf(stderr, "%d\n", tp_i->plen);

    //log_print_buffer(LOG_INFO, "Test", tp_i->buf->ptr, tp_i->buf->len);
    //log_print_buffer(LOG_INFO, "Test", tp_i->buf2->ptr, tp_i->buf2->len);

    log_buf(LOG_INFO, "Test", tp_i->buf->ptr, tp_i->buf->len);
    log_buf(LOG_INFO, "Test", tp_i->buf2->ptr, tp_i->buf2->len);
    log_buf(LOG_INFO, "SDU", tp_i->sdu->ptr, tp_i->sdu->len);

    return 0;
}