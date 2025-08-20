#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

void process_b(int read_fd) {
    char buffer[256];

    while (1) {
        ssize_t bytes_read = read(read_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Process B received: %s", buffer);
            fflush(stdout);
        } else if (bytes_read == 0) {
            break;
        }
    }
}

void process_c(int read_fd) {
    char buffer[256];

    while (1) {
        ssize_t bytes_read = read(read_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Process C received: %s", buffer);
            fflush(stdout);
        } else if (bytes_read == 0) {
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <A|B|C> [pipe_fd]\n", argv[0]);
        return 1;
    }

    char option = argv[1][0];

    switch (option) {
        case 'A': {
            int pipe_b[2], pipe_c[2];
            if (pipe(pipe_b) == -1 || pipe(pipe_c) == -1) {
                perror("pipe failed");
                exit(1);
            }

            printf("Main process started. Type messages (Ctrl+C to exit):\n");

            pid_t pid_b = fork();
            if (pid_b == 0) {
                close(pipe_b[1]); // 关闭写端
                close(pipe_c[0]); // 关闭另一个管道
                close(pipe_c[1]);
                char fd_str[10];
                sprintf(fd_str, "%d", pipe_b[0]);
                execl(argv[0], argv[0], "B", fd_str, NULL);
                perror("execl failed for process B");
                exit(1);
            }

            pid_t pid_c = fork();
            if (pid_c == 0) {
                close(pipe_c[1]); // 关闭写端
                close(pipe_b[0]); // 关闭另一个管道
                close(pipe_b[1]);
                char fd_str[10];
                sprintf(fd_str, "%d", pipe_c[0]);
                execl(argv[0], argv[0], "C", fd_str, NULL);
                perror("execl failed for process C");
                exit(1);
            }

            close(pipe_b[0]); // 主进程关闭读端
            close(pipe_c[0]);

            char input[256];
            while (fgets(input, sizeof(input), stdin)) {
                write(pipe_b[1], input, strlen(input));
                write(pipe_c[1], input, strlen(input));
            }

            close(pipe_b[1]);
            close(pipe_c[1]);

            kill(pid_b, SIGTERM);
            kill(pid_c, SIGTERM);

            int status;
            while (wait(&status) > 0);

            printf("Main process exiting\n");
            break;
        }

        case 'B':
            if (argc < 3) {
                fprintf(stderr, "Missing pipe file descriptor\n");
                return 1;
            }
            process_b(atoi(argv[2]));
            break;

        case 'C':
            if (argc < 3) {
                fprintf(stderr, "Missing pipe file descriptor\n");
                return 1;
            }
            process_c(atoi(argv[2]));
            break;

        default:
            fprintf(stderr, "Invalid option: %c. Use A, B, or C\n", option);
            return 1;
    }

    return 0;
}