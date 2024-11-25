//
// Created by root on 5/30/24.
//

#include "ld_utils.h"
#include "ld_buffer.h"


/* TODO: 处理一下和km_src的关系 */
void generate_rand(uint8_t *rand, size_t len) {
//#ifdef  USE_CRYCARD
//    km_generate_random(rand, len);
//#elif UNUSE_CRYCARD
    rand_bytes(rand, len);
//#endif
}

/* generate a rand int, max size is 64bits (8 bytes) */
uint64_t generate_urand(size_t rand_bits_sz) {
    if (rand_bits_sz > SYSTEM_BITS) return 0;
    uint64_t ret = 0;

    uint8_t rand[8] = {0};
    generate_rand(rand, 8);

    for (int i = 0; i < 8; i++) {
        ret += rand[i] << (BITS_PER_BYTE * (7 - i));
    }
    return ret & 0xFFFFFFFF >> (SYSTEM_BITS - rand_bits_sz);
}

/* generate a unlimit rand array */
void generate_nrand(uint8_t *rand, size_t sz) {
    generate_rand(rand, sz);
}

void get_time(char *time_str, enum TIME_MOD t_mod) {
    struct tm tm;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    time_str[strftime(time_str, 32, time_format[t_mod], &tm)] = '\0';
}

int32_t memrchr(const buffer_t *buf, uint8_t target) {
    int i = 0;
    while (i < buf->len) {
        if (buf->ptr[i] != target) {
            return i;
        }
        i++;
    }
    return -1; // 表示没有找到不为0的字符
}

int bytes_len(size_t n) {
    int i = 0;
    n--;
    while (n >> i++ != 0);
    return i - 1;
}

