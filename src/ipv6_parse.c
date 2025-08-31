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

#include "ld_santilizer.h"


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
