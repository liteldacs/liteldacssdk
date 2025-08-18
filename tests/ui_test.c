#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "ld_log.h"

void print_to_window(WINDOW *win, const char *text) {
    static int line = 0;
    int height = getmaxy(win);

    if(line >= height-1) {
        scroll(win);
        line = height-2;
    }

    mvwprintw(win, line++, 0, "%s", text);
    wrefresh(win);
}

int main() {
    initscr();
    cbreak();
    noecho();

    int height, width;
    getmaxyx(stdscr, height, width);

    WINDOW *upper_win = newwin(height/2, width, 0, 0);
    WINDOW *lower_win = newwin(height/2, width, height/2, 0);

    scrollok(lower_win, TRUE);

    mvwprintw(upper_win, height/4, (width - 11)/2, "Hello world\n");
    wrefresh(upper_win);

    // 创建管道
    int pipefd[2];
    pipe(pipefd);

    if(fork() == 0) {
        // 子进程：重定向stdout到管道并执行printf
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        // 现在可以直接使用printf
        for(int i = 1; i <= 10; i++) {
            log_warn("Printf line %d: Hello from child process", i);
            // fflush(stdout);
            sleep(1);
        }
        exit(0);
    } else {
        // 父进程：从管道读取并显示
        close(pipefd[1]);
        FILE *pipe_read = fdopen(pipefd[0], "r");

        char buffer[256];
        while(fgets(buffer, sizeof(buffer), pipe_read)) {
            print_to_window(lower_win, buffer);
        }

        fclose(pipe_read);
        wait(NULL);
    }

    getch();

    delwin(upper_win);
    delwin(lower_win);
    endwin();

    return 0;
}