//
// Created by 邹嘉旭 on 2024/4/22.
//
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include "../global/ldacs_sim.h"

l_err check_path(const char *dir_path) {
    struct stat st;

    if (stat(dir_path, &st) == 0) {
        // 如果成功获取到状态，检查是否为目录
        if (S_ISDIR(st.st_mode))    return LD_OK;

        return LD_ERR_WRONG_PATH; // 路径存在但不是目录，返回失败
    }
    // 如果是路径不存在，尝试创建目录
    if (errno == ENOENT) {
        if (mkdir(dir_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)    return LD_OK; // 目录创建成功，返回成功

        perror("mkdir");
        return LD_ERR_WRONG_PATH; // 创建目录失败，返回失败
    }
    perror("stat");
    return LD_ERR_WRONG_PATH; // 其他错误，返回失败
}

int main() {
    const char *dir_to_create = "../../log2";
    int result = check_path(dir_to_create);

    if (result == 0) {
        printf("Directory operation completed successfully.\n");
    } else {
        printf("Directory operation failed.\n");
    }

    return 0;
}