//
// Created by jiaxv on 25-8-19.
//
#include "ipv6_parse.h"
#include "ldacs_sim.h"
#include "ld_net.h"
#include "ld_santilizer.h"

#define V6_ADDR_LEN 16

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