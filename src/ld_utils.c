//
// Created by root on 5/30/24.
//

#include "ld_utils.h"
#include "ld_buffer.h"


void get_time(char *time_str, enum TIME_MOD t_mod, enum TIME_PREC t_pesc) {
    struct tm tm;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    switch (t_pesc) {
        case LD_TIME_DAY:
        case LD_TIME_SEC: {
            time_str[strftime(time_str, 32, time_format[t_mod][t_pesc], &tm)] = '\0';
            break;
        }
        case LD_TIME_MICRO: {
            char pre_time[32] = {0};
            pre_time[strftime(pre_time, 32, time_format[t_mod][LD_TIME_SEC], &tm)] = '\0';
            snprintf(time_str, 64, "%s.%06ld\0", pre_time, tv.tv_usec);
            break;
        }
    }
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

int ld_split(const char *str, char ***argv) {
    // 统计单词数量
    int count = 0;
    const char *p = str;
    while (*p != '\0') {
        while (*p == ' ') p++;
        if (*p == '\0') break;
        count++;
        while (*p != ' ' && *p != '\0') p++;
    }

    // 分配指针数组，多一个位置存放NULL
    *argv = (char **)malloc(sizeof(char *) * (count + 1));
    if (!*argv) return 0;

    int index = 0;
    const char *start = str;
    while (*start != '\0') {
        while (*start == ' ') start++;
        if (*start == '\0') break;
        // 找到单词的结束位置
        const char *end = start;
        while (*end != ' ' && *end != '\0') end++;
        // 计算单词长度并分配内存
        int len = end - start;
        char *token = (char *)malloc(len + 1);
        if (!token) {
            // 分配失败，释放已分配内存
            for (int i = 0; i < index; i++) free((*argv)[i]);
            free(*argv);
            return 0;
        }
        // 复制单词并终止
        strncpy(token, start, len);
        token[len] = '\0';
        (*argv)[index++] = token;
        // 移动到下一个位置
        start = end;
    }
    (*argv)[index] = NULL; // 数组以NULL结尾
    return count;
}