//
// Created by 35270 on 25-6-12.
//

#ifndef IPV6_PARSE_H
#define IPV6_PARSE_H
#include "ld_buffer.h"
#include "ld_santilizer.h"


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

static void free_ipv6_tcp(void *args) {
    ipv6_tcp_t *pkt = args;
    if (pkt) {
        if (pkt->src_address) {
            free_buffer(pkt->src_address);
        }
        if (pkt->dst_address) {
            free_buffer(pkt->dst_address);
        }
        if (pkt->data) {
            free_buffer(pkt->data);
        }
        free(pkt);
    }
}

#pragma pack()

extern struct_desc_t ipv6_tcp_desc;
#endif //IPV6_PARSE_H
