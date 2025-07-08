//
// Created by 35270 on 25-6-12.
//

#ifndef IPV6_PARSE_H
#define IPV6_PARSE_H
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
int parse_ipv6_udp_packet(const unsigned char *packet, int packet_len,char* SourceIP,char* DestinationIP,char* SourcePN,char* DestinationPN);
// 构造 IPv6 UDP 数据报文
typedef struct ipv6_hdr {
    uint32_t version_tc_fl;
    uint16_t payload_len;
    uint8_t  next_header;
    uint8_t  hop_limit;
    struct in6_addr saddr;
    struct in6_addr daddr;
} ipv6_hdr;

typedef struct udp_hdr {
    uint16_t source;
    uint16_t dest;
    uint16_t len;
    uint16_t check;
} udp_hdr;
int construct_ipv6_udp_packet_to_char(char *dest_ip, char *src_ip, char * dest_port, char * src_port, char *data, int data_len_subByte, char *packet);
void ipv6_hex_to_colon(const char *hex_ip, char *colon_ip);
void ipv6_colon_to_hex(const char *colon_ip, char *hex_ip);
void ipv6_expand(const char *compressed_ip, char *full_ip);
void ipv6_compress(const char *full_ip, char *compressed_ip);
#endif //IPV6_PARSE_H
