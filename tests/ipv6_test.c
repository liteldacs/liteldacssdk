//
// Created by jiaxv on 25-8-19.
//
#include "ldacs_sim.h"
#include "ld_santilizer.h"

#define V6_ADDR_LEN 16

#pragma pack(1)
typedef struct ipv6_tcp_s {
    uint8_t version;
    uint8_t traffic_class;
    uint32_t flow_label;
    uint16_t payload_len;
    uint8_t next_header;
    uint8_t hop_limit;
    buffer_t *src_address;
    buffer_t *dst_address;
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t sqn;
    uint32_t ack;
    uint8_t bias;
    uint8_t reserve;
    uint16_t flag;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
    buffer_t *data;
} ipv6_tcp_t;
#pragma pack()

static field_desc ipv6_tcp_fields[] = {
    {ft_set, 4, "VERSION", NULL},
    {ft_set, 8, "TRAFFIC CLASS", NULL},
    {ft_set, 20, "FLOW LABEL", NULL},
    {ft_set, 16, "PAYLOAD LEN", NULL},
    {ft_set, 8, "NEXT HEADER", NULL},
    {ft_set, 8, "HOP LIMIT", NULL},
    {ft_pad, 0, "PAD", NULL},
    {ft_fl_str, 0, "IP SRC", &(pk_fix_length_t){.len = 16}},
    {ft_fl_str, 0, "IP DST", &(pk_fix_length_t){.len = 16}},
    {ft_set, 16, "SRC PORT", NULL},
    {ft_set, 16, "DST PORT", NULL},
    {ft_set, 32, "SQN", NULL},
    {ft_set, 32, "ACK", NULL},
    {ft_set, 4, "BIAS", NULL},
    {ft_set, 3, "PRESERVE", NULL},
    {ft_set, 9, "FLAG", NULL},
    {ft_set, 16, "WINDOW", NULL},
    {ft_set, 16, "CHECKSUM", NULL},
    {ft_set, 16, "URGENT", NULL},
    {ft_pad, 0, "PAD", NULL},
    {ft_dl_str, 0, "DATA", NULL},
    {ft_pad, 0, "PAD", NULL},
    {ft_end, 0, NULL, NULL},
};
struct_desc_t ipv6_tcp_desc = {"TCP V6", ipv6_tcp_fields};


int main() {
    ipv6_tcp_t v6 = {
        .version = 0x6,
        .traffic_class = 0x3,
        .flow_label = 0x1,
        .payload_len = 11,
        .next_header = 0x7,
        .hop_limit = 0x10,
        .src_address = init_buffer_unptr(),
        .dst_address = init_buffer_unptr(),
        .src_port = 9456,
        .dst_port = 9789,
        .sqn = 1000,
        .ack = 2000,
        .bias = 0,
        .reserve = 0x1,
        .flag = 0x2,
        .window = 500,
        .checksum = 0xABCD,
        .urgent = 0,
        .data = init_buffer_unptr()
    };

    struct in6_addr addr;
    inet_pton(AF_INET6, "3::1", &addr);

    CLONE_TO_CHUNK(*v6.src_address, addr.__in6_u.__u6_addr8, 16);
    CLONE_TO_CHUNK(*v6.dst_address, addr.__in6_u.__u6_addr8, 16);

    CLONE_TO_CHUNK(*v6.data, "hello world", 11);
    buffer_t *buf = gen_pdu(&v6, &ipv6_tcp_desc, "TCP V6");

    log_buf(LOG_INFO, "IPV6", buf->ptr, buf->len);
}