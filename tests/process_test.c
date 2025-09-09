#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define CHILD_EXEC_PATH "./ldacs-combine" // 定义子进程可执行文件的路径
#define HEARTBEAT_INTERVAL 2               // 心跳间隔（秒）

int main() {
    int pipefd[2];
    pid_t cpid;
    char buf;

    // 1. 创建管道
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // 2. 创建子进程
    cpid = fork();
    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0) { // 子进程代码
        // 关闭子进程中不需要的读端
        close(pipefd[0]);

        if (chdir("/home/jiaxv/ldacs/ldacs-combine/cmake-build-debug/bin") != 0) {
            perror("chdir");
            _exit(EXIT_FAILURE);
        }

        // 准备传递给execl的参数
        // 将写端文件描述符转换为字符串
        char write_fd_str[16];
        snprintf(write_fd_str, sizeof(write_fd_str), "%d", pipefd[1]);

        // 使用execl执行位于不同路径的子进程程序
        // argv[0] 通常是程序名，argv[1] 是我们传递的文件描述符
        execl(CHILD_EXEC_PATH, "ldacs-combine", "-c", "../../config/ldacs_config_as_1.yaml", "-f", write_fd_str, (char *)NULL);
.
        // 如果execl返回，说明执行失败
        perror("execl failed");
        close(pipefd[1]); // 关闭写端（虽然execl失败才会执行到这里）
        _exit(EXIT_FAILURE);
    } else { // 父进程代码
        printf("父进程 PID: %d\n", getpid());
        printf("启动的子进程 PID: %d\n", cpid);

        // 关闭父进程中不需要的写端
        close(pipefd[1]);

        // 3. 父进程循环读取管道
        printf("父进程开始监听心跳...\n");
        while (1) {
            ssize_t num_read = read(pipefd[0], &buf, 1);
            if (num_read > 0) {
                // 逐字节读取并打印，直到遇到换行符
                // putchar(buf);
                // if (buf == '\n') {
                //     fflush(stdout); // 确保立即输出
                // }
            } else if (num_read == 0) {
                // 读端返回0表示写端已关闭，子进程可能已退出
                printf("检测到管道关闭，子进程可能已结束。\n");
                break;
            } else {
                perror("read");
                break;
            }
        }

        // 关闭读端
        close(pipefd[0]);

        // 等待子进程结束，获取退出状态
        int wstatus;
        if (waitpid(cpid, &wstatus, 0) == -1) {
            perror("waitpid");
        } else {
            if (WIFEXITED(wstatus)) {
                printf("子进程正常退出，退出码: %d\n", WEXITSTATUS(wstatus));
            } else if (WIFSIGNALED(wstatus)) {
                printf("子进程被信号终止，信号码: %d\n", WTERMSIG(wstatus));
            }
        }
    }

    return 0;
}



