//
// Created by 邹嘉旭 on 2025/3/5.
//
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>  // for inet_pton
#include <utils/ld_log.h>

int main() {
    const char *ipv6_str = "fe01::1";
    unsigned char ipv6_bin[16];  // 16 bytes for IPv6 address

    // Convert IPv6 string to binary
    if (inet_pton(AF_INET6, ipv6_str, ipv6_bin) != 1) {
        perror("inet_pton");
        return EXIT_FAILURE;
    }

    log_buf(LOG_INFO, "IPV6 BIN", ipv6_bin, 16);

    // Print the binary representation
    printf("IPv6 address in binary: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", ipv6_bin[i]);
    }
    printf("\n");

    return EXIT_SUCCESS;
}
