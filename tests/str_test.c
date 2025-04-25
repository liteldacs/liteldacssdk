//
// Created by 邹嘉旭 on 2025/4/25.
//
#include "ld_log.h"
#include "ld_utils.h"
int main() {
    const char *str = "Hello World nihao nihao";
    char **argv;
    int args = ld_split(str, &argv);
    log_warn("!!! %d", args);
    if (argv) {
        for (int i = 0; argv[i] != NULL; i++) {
            printf("%s\n", argv[i]);
            free(argv[i]); // 释放每个单词内存
        }
        free(argv); // 释放指针数组
    }
    return 0;
}