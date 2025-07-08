//
// Created by 邹嘉旭 on 2025/7/8.
//

#include "ipv6_parse.h"
#include "ld_log.h"

int main() {
    const char *data = "ABBA";
    const char pkt[2048] = {0};
    int pkt_len =  construct_ipv6_udp_packet_to_char("2001::100", "2001::200", "5000", "5001", data, 4, pkt);

    log_buf(LOG_WARN, "MARSHALED PKT", pkt, pkt_len);

    const char src_ip[32] = {0};
    const char dst_ip[32] = {0};
    const char src_port[16] = {0};
    const char dst_port[16] = {0};

    parse_ipv6_udp_packet(pkt, pkt_len, src_ip, dst_ip, src_port, dst_port);

    log_debug("UNMARSHALED PKT src_ip: %s:%s, dst_ip: %s:%s,", src_ip, src_port, dst_ip, dst_port);

}
