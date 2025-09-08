//
// Created by root on 4/16/25.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include "ld_config.h"
#include <key_manage.h>
#include <kmdb.h>
#include <gmssl/rand.h>

int main() {
    for (int i = 1; i <= 10; i++) {
        uint8_t msg[1500];
        size_t len = i * 100;
        size_t curr = 0;
        for (int j = 0; j < len / RAND_BYTES_MAX_SIZE; j++) {
            km_generate_random(msg + curr, RAND_BYTES_MAX_SIZE);
            curr += RAND_BYTES_MAX_SIZE;
        }
        km_generate_random(msg + curr, len % RAND_BYTES_MAX_SIZE);

        log_buf(LOG_DEBUG, "AAA", msg, len);
        sleep(1);
    }
    return 0;
}