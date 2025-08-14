#include <ld_log.h>
#include <event2/event.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

// 全局 event_base
struct event_base *base;

// 子线程函数
void *thread_func(void *arg) {
    ld_aqueue_tStarting event loop in thread\n");
    event_base_dispatch(base);
    ld_aqueue_tEvent loop ended in thread\n");
    return NULL;
}

// 事件回调函数
void timeout_cb(evutil_socket_t fd, short event, void *arg) {
    ld_aqueue_tTimeout occurred\n");
    sleep(5);
}

int main() {
    // 创建 event_base
    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not initialize libevent\n");
        return 1;
    }

    pthread_t thread;
    // 创建子线程
    pthread_create(&thread, NULL, thread_func, NULL);

    // 模拟延迟，等待子线程启动事件循环
    sleep(1);

    // 添加事件
    struct timeval tv = {2, 0}; // 2 秒超时
    struct event *timeout_event = event_new(base, -1, EV_TIMEOUT, timeout_cb, NULL);
    event_add(timeout_event, &tv);
    printf("Timeout event added\n");

    // 等待子线程结束
    // pthread_join(thread, NULL);
    sleep(10086);

    // 清理
    event_free(timeout_event);
    event_base_free(base);

    return 0;
}
