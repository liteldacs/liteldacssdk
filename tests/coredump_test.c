#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

void sigsegv_handler(int signo, siginfo_t *info, void *context) {
    fprintf(stderr, "Received SIGSEGV at address %p\n", info->si_addr);
    fprintf(stderr, "Attempting to generate core dump...\n");
    
    // 记录更多错误信息到日志
    FILE *log = fopen("crash.log", "a");
    if (log) {
        fprintf(log, "Crash at %p, errno=%d (%s)\n", 
                info->si_addr, errno, strerror(errno));
        fclose(log);
    }
    
    // 恢复默认处理并重新触发
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &sa, NULL);
    kill(getpid(), SIGSEGV);
}

int main() {
    struct sigaction sa;
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sigsegv_handler;
    sa.sa_flags = SA_SIGINFO;
    
    sigaction(SIGSEGV, &sa, NULL);
    
    //int *p = calloc(1, sizeof(int));
    //free(p);

    // 人为制造段错误
    int *p = NULL;
    *p = 1;

    return 0;
}