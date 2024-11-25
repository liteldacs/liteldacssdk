//
// Created by root on 5/30/24.
//

#include "ld_utils.h"
#include "ld_buffer.h"


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

