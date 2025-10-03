#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#include "ld_log.h"

#define CHILD_EXEC_PATH "./ldacs-combine" // 定义子进程可执行文件的路径
#define NUM_CHILDREN 39                     // 子进程数量

typedef int pipe_fd_t[2];

pid_t cpids[NUM_CHILDREN];

static void sigint_handler(int signum) {

    // 终止所有子进程
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (cpids[i] > 0) {
            printf("正在终止子进程 PID: %d\n", cpids[i]);
            kill(cpids[i], SIGKILL);
        }
    }
    sleep(1);

    log_info("multiple as simulator(PID: %u) exit...", getpid());
    exit(0);
}

void init_signal() {
    signal(SIGINT, sigint_handler);
    signal(SIGABRT, sigint_handler);
    signal(SIGQUIT, sigint_handler);
    signal(SIGTERM, sigint_handler);
    signal(SIGPIPE, SIG_IGN); // client close, server write will recv sigpipe
}

int main() {
    pipe_fd_t pipefds[NUM_CHILDREN];
    char buf;
    init_signal();
    printf("父进程 PID: %d\n", getpid());

    // 1. 创建多个管道
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (pipe(pipefds[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // 2. 创建多个子进程
    for (int i = 0; i < NUM_CHILDREN; i++) {
        usleep(100000);
        cpids[i] = fork();
        if (cpids[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (cpids[i] == 0) { // 子进程代码
            // 关闭子进程中不需要的读端
            for (int j = 0; j < NUM_CHILDREN; j++) {
                close(pipefds[j][0]);
            }

            if (chdir("/home/jiaxv/ldacs/ldacs-combine/cmake-build-debug/bin") != 0) {
                perror("chdir");
                _exit(EXIT_FAILURE);
            }

            // 准备传递给execl的参数
            // 将写端文件描述符转换为字符串
            char write_fd_str[16];
            snprintf(write_fd_str, sizeof(write_fd_str), "%d", pipefds[i][1]);

            // 为不同子进程使用不同的配置文件
            char config_path[64];
            snprintf(config_path, sizeof(config_path), "../../config/ldacs_config_as_%d.yaml", i+2);

            execl(CHILD_EXEC_PATH, "ldacs-combine", "-c", config_path, "-D", "-E", "-f", write_fd_str, (char *)NULL);

            perror("execl failed");
            // 关闭写端（虽然execl失败才会执行到这里）
            for (int j = 0; j < NUM_CHILDREN; j++) {
                close(pipefds[j][1]);
            }
            _exit(EXIT_FAILURE);
        }
    }


    // 关闭父进程中不需要的写端
    for (int i = 0; i < NUM_CHILDREN; i++) {
        close(pipefds[i][1]);
    }

    // 父进程循环读取所有管道
    int active_children = NUM_CHILDREN;
    // while (active_children > 0) {
    //     for (int i = 0; i < NUM_CHILDREN; i++) {
    //         if (cpids[i] != -1) { // 检查子进程是否还在运行
    //             ssize_t num_read = read(pipefds[i][0], &buf, 1);
    //             if (num_read > 0) {
    //                 // 逐字节读取并打印，直到遇到换行符
    //                 putchar(buf);
    //                 if (buf == '\n') {
    //                     fflush(stdout); // 确保立即输出
    //                 }
    //             } else if (num_read == 0) {
    //                 // 读端返回0表示写端已关闭，子进程可能已退出
    //                 printf("检测到子进程 %d 的管道关闭，子进程可能已结束。\n", i+1);
    //                 cpids[i] = -1; // 标记该子进程已退出
    //                 active_children--;
    //             } else {
    //                 // 检查是否是错误导致的
    //                 if (errno != EAGAIN && errno != EWOULDBLOCK) {
    //                     perror("read");
    //                 }
    //             }
    //         }
    //     }
    // }

    // 关闭读端
    for (int i = 0; i < NUM_CHILDREN; i++) {
        close(pipefds[i][0]);
    }

    // 等待所有子进程结束，获取退出状态
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (cpids[i] != -1) { // 只等待仍然有效的子进程
            int wstatus;
            if (waitpid(cpids[i], &wstatus, 0) == -1) {
                perror("waitpid");
            } else {
                if (WIFEXITED(wstatus)) {
                    printf("子进程 %d 正常退出，退出码: %d\n", i+1, WEXITSTATUS(wstatus));
                } else if (WIFSIGNALED(wstatus)) {
                    printf("子进程 %d 被信号终止，信号码: %d\n", i+1, WTERMSIG(wstatus));
                }
            }
        }
    }    return 0;
}



