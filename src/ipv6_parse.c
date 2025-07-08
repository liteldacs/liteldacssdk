//
// Created by 35270 on 25-6-12.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include "ipv6_parse.h"


void construct_ipv6_udp_packet(char *dest_ip, char *src_ip, uint16_t dest_port, uint16_t src_port, char *data,
                               int data_len_Byte, char *packet);

uint16_t udp_checksum(const void *buff, size_t len, struct in6_addr src_addr, struct in6_addr dest_addr);

// 解析 IPv6 UDP 报文
int parse_ipv6_udp_packet(const unsigned char *packet, int packet_len, char *SourceIP, char *DestinationIP,
                          char *SourcePN, char *DestinationPN) {
    if (packet_len < sizeof(struct ip6_hdr) + sizeof(struct udphdr)) {
        printf("Packet length is too short to be a valid IPv6 UDP packet.\n");
        return -1;
    } else {
        // 解析 IPv6 头部
        struct ip6_hdr *ip6_header = (struct ip6_hdr *) packet;

        char src_ip_str[INET6_ADDRSTRLEN];
        char dst_ip_str[INET6_ADDRSTRLEN];

        // 将源 IP 地址转换为字符串
        inet_ntop(AF_INET6, &(ip6_header->ip6_src), src_ip_str, INET6_ADDRSTRLEN);
        // 将目的 IP 地址转换为字符串
        inet_ntop(AF_INET6, &(ip6_header->ip6_dst), dst_ip_str, INET6_ADDRSTRLEN);

        // 计算 UDP 头部的位置
        struct udphdr *udp_header = (struct udphdr *) (packet + sizeof(struct ip6_hdr));

        // 提取端口号并转换为主机字节序
        uint16_t src_port = ntohs(udp_header->source);
        uint16_t dst_port = ntohs(udp_header->dest);

        // // 输出解析结果
        // printf("Source IP: %s\n", src_ip_str);
        // printf("Destination IP: %s\n", dst_ip_str);
        // printf("Source Port: %u\n", src_port);
        // printf("Destination Port: %u\n", dst_port);
        strcpy(SourceIP, src_ip_str);
        strcpy(DestinationIP, dst_ip_str);
        sprintf(DestinationPN, "%u", (unsigned int) dst_port);
        sprintf(SourcePN, "%u", (unsigned int) src_port);
        return 0;
    }
}

// 计算 UDP 校验和
uint16_t udp_checksum(const void *buff, size_t len, struct in6_addr src_addr, struct in6_addr dest_addr) {
    const uint16_t *buf = buff;
    uint32_t sum;
    size_t length = len;

    // 伪首部
    uint16_t *pseudo_header = (uint16_t *) &src_addr;
    for (int i = 0; i < 8; i++) {
        sum += *pseudo_header++;
    }
    pseudo_header = (uint16_t *) &dest_addr;
    for (int i = 0; i < 8; i++) {
        sum += *pseudo_header++;
    }
    uint32_t udp_length = htons(len);
    sum += (uint16_t) (udp_length >> 16) + (uint16_t) (udp_length & 0xffff);
    sum += IPPROTO_UDP;

    // UDP 数据
    while (length > 1) {
        sum += *buf++;
        length -= 2;
    }
    if (length == 1) {
        sum += *(uint8_t *) buf;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (uint16_t) (~sum);
}

// 构造 IPv6 UDP 数据报文
void construct_ipv6_udp_packet(char *dest_ip, char *src_ip, uint16_t dest_port, uint16_t src_port, char *data,
                               int data_len_subByte, char *packet) {
    ipv6_hdr ipv6_hdr;
    udp_hdr udp_hdr;
    // 解析 IP 地址
    inet_pton(AF_INET6, dest_ip, &ipv6_hdr.daddr);
    inet_pton(AF_INET6, src_ip, &ipv6_hdr.saddr);

    // 填充 IPv6 首部
    ipv6_hdr.version_tc_fl = htonl((6 << 28) | (0 << 20) | 0);
    uint16_t udp_payload_len = (uint16_t) data_len_subByte;
    ipv6_hdr.payload_len = htons(sizeof(udp_hdr) + udp_payload_len);
    ipv6_hdr.next_header = IPPROTO_UDP;
    ipv6_hdr.hop_limit = 64;

    // 填充 UDP 首部
    udp_hdr.source = htons(src_port);
    udp_hdr.dest = htons(dest_port);
    udp_hdr.len = htons(sizeof(udp_hdr) + udp_payload_len);
    udp_hdr.check = 0; // 先置零，用于计算校验和

    // 计算 UDP 校验和
    char udp_packet[sizeof(udp_hdr) + udp_payload_len];
    memcpy(udp_packet, &udp_hdr, sizeof(udp_hdr));
    memcpy(udp_packet + sizeof(udp_hdr), data, udp_payload_len);
    udp_hdr.check = udp_checksum(udp_packet, sizeof(udp_hdr) + udp_payload_len, ipv6_hdr.saddr, ipv6_hdr.daddr);

    // 构造最终报文
    memcpy(packet, &ipv6_hdr, sizeof(ipv6_hdr));
    memcpy(packet + sizeof(ipv6_hdr), &udp_hdr, sizeof(udp_hdr));
    memcpy(packet + sizeof(ipv6_hdr) + sizeof(udp_hdr), data, udp_payload_len);
}

// 计算真实 IPv6 UDP 报文长度（单位为 byte）
size_t calculate_ipv6_udp_length(const char *data) {
    // IPv6 固定头部长度（字节）
    const size_t ipv6_header_length = 40;
    // UDP 头部长度（字节）
    const size_t udp_header_length = 8;
    // UDP 数据长度（字节）
    size_t udp_data_length = strlen(data);

    // 计算 UDP 报文总长度（字节）
    size_t udp_total_length = udp_header_length + udp_data_length;
    // 计算 IPv6 报文总长度（字节）
    size_t ipv6_total_length = ipv6_header_length + udp_total_length;

    // 转换为比特数
    return ipv6_total_length;
}

int construct_ipv6_udp_packet_to_char(char *dest_ip, char *src_ip, char *dest_port, char *src_port, char *data,
                                      int data_len_subByte, char *packet) {
    size_t packet_len_byte = calculate_ipv6_udp_length(data);
    construct_ipv6_udp_packet(dest_ip, src_ip, atoi(dest_port), atoi(src_port), data, data_len_subByte, packet);
    return (int) packet_len_byte;
}

void ipv6_colon_to_hex(const char *colon_ip, char *hex_ip) {
    struct in6_addr addr;
    if (inet_pton(AF_INET6, colon_ip, &addr) != 1) {
        fprintf(stderr, "Invalid IPv6 address\n");
        return;
    }
    for (int i = 0; i < 16; i++) {
        sprintf(hex_ip + i * 2, "%02x", addr.s6_addr[i]);
    }
}

void ipv6_hex_to_colon(const char *hex_ip, char *colon_ip) {
    struct in6_addr addr;
    char temp[36];
    if (strlen(hex_ip) != 32) {
        fprintf(stderr, "Invalid hexadecimal IPv6 address length\n");
        return;
    }
    for (int i = 0; i < 16; i++) {
        sscanf(hex_ip + i * 2, "%2hhx", &addr.s6_addr[i]);
    }
    if (inet_ntop(AF_INET6, &addr, colon_ip, INET6_ADDRSTRLEN) == NULL) {
        fprintf(stderr, "Conversion failed\n");
    }
    // ipv6_compress(temp,colon_ip);
}

// 将完整 IPv6 地址转换为简化形式
void ipv6_compress(const char *full_ip, char *compressed_ip) {
    struct in6_addr addr;
    if (inet_pton(AF_INET6, full_ip, &addr) != 1) {
        fprintf(stderr, "Invalid IPv6 address\n");
        return;
    }

    // 将 128 位地址按 16 位分组
    uint16_t parts[8];
    for (int i = 0; i < 8; i++) {
        parts[i] = (addr.s6_addr[i * 2] << 8) | addr.s6_addr[i * 2 + 1];
    }

    int start = -1, max_len = 0;
    int current_start = -1, current_len = 0;

    // 查找最长连续 0 字段
    for (int i = 0; i < 8; i++) {
        if (parts[i] == 0) {
            if (current_start == -1) {
                current_start = i;
            }
            current_len++;
        } else {
            if (current_len > max_len) {
                max_len = current_len;
                start = current_start;
            }
            current_start = -1;
            current_len = 0;
        }
    }

    // 检查最后一段连续 0
    if (current_len > max_len) {
        max_len = current_len;
        start = current_start;
    }

    int pos = 0;
    // 处理开头到连续 0 之前的部分
    for (int i = 0; i < start; i++) {
        if (i > 0) {
            compressed_ip[pos++] = ':';
        }
        pos += sprintf(compressed_ip + pos, "%x", parts[i]);
    }

    // 添加双冒号
    if (start != -1) {
        if (start == 0 || (start == 8 - max_len && pos == 0)) {
            compressed_ip[pos++] = ':';
        }
        compressed_ip[pos++] = ':';
    }

    // 处理连续 0 之后的部分
    for (int i = start + max_len; i < 8; i++) {
        if (i > start + max_len && (start != -1 || i > 0)) {
            compressed_ip[pos++] = ':';
        }
        pos += sprintf(compressed_ip + pos, "%x", parts[i]);
    }

    compressed_ip[pos] = '\0';
}

// 将简化 IPv6 地址转换为完整形式
void ipv6_expand(const char *compressed_ip, char *full_ip) {
    struct in6_addr addr;
    // 将简化 IPv6 地址转换为网络字节序二进制格式
    if (inet_pton(AF_INET6, compressed_ip, &addr) != 1) {
        fprintf(stderr, "Invalid IPv6 address\n");
        return;
    }
    // 将网络字节序二进制格式转换为完整 IPv6 地址字符串
    if (inet_ntop(AF_INET6, &addr, full_ip, INET6_ADDRSTRLEN) == NULL) {
        fprintf(stderr, "Conversion failed\n");
    }
}
