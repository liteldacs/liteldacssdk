//
// Created by 邹嘉旭 on 2024/12/30.
//

#include "ld_newtimer.h"

/*
 * 根据纳秒计算微妙
 * @param timev
 * @param nano_time
 */
static void nano_to_timeval(struct timeval *timev, const uint64_t nano_time) {
    timev->tv_sec = nano_time / SECOND;
    timev->tv_usec = (nano_time % SECOND) / MICROSECOND;
}

static void *gtimer_event_dispatch(void *args) {
    ld_gtimer_handler_t *gtimer = args;
    while (1) {
        struct epoll_event events[10];
        int num_events = epoll_wait(gtimer->epoll_fd, events, 10, -1); // 阻塞等待事件
        if (num_events == -1) {
            perror("epoll_wait");
            return NULL;
        }

        uint64_t expirations;
        for (int i = 0; i < num_events; i++) {
            for (int j = 0; j < gtimer->node_count; j++) {
                timer_node_t *node = &gtimer->nodes[j];
                if (events[i].data.fd == node->timer_fd) {
                    ssize_t ret = read(node->timer_fd, &expirations, sizeof(expirations));
                    if (ret != sizeof(expirations)) {
                        perror("read");
                        return NULL;
                    }

                    for (int k = 0; k < node->cb_count; k++) {
                        gtimer_cb_t *cbt = &node->cbs[k];
                        if (cbt->to_times == TIMER_INFINITE || cbt->has_times < cbt->to_times) {
                            pthread_create(&cbt->th, NULL, cbt->cb, cbt->args);
                            pthread_detach(cbt->th);
                            cbt->has_times++;
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

static void *stimer_event_dispatch(void *arg) {
    ld_stimer_handler_t *stimer = calloc(1, sizeof(ld_stimer_handler_t));
    stimer->cb = ((ld_stimer_handler_t *)arg)->cb;
    stimer->args = ((ld_stimer_handler_t *)arg)->args;
    stimer->nano = ((ld_stimer_handler_t *)arg)->nano;

    struct timeval tv;

    evthread_use_pthreads();
    struct event_base *base = event_base_new();
    nano_to_timeval(&tv, stimer->nano);

    struct event *ev = event_new(base, -1, 0, stimer->cb, stimer->args);
    if (!ev) {
        fprintf(stderr, "Could not create event!\n");
        return NULL;
    }
    event_add(ev, &tv);
    event_base_dispatch(base);

    // 清理资源
    event_free(ev);
    event_base_free(base);

    free(stimer);

    return NULL;
}

l_err init_gtimer(ld_gtimer_handler_t *gtimer) {
    memset(gtimer, 0, sizeof(ld_gtimer_handler_t));

    // 创建 epoll 实例
    gtimer->epoll_fd = epoll_create1(0);
    if (gtimer->epoll_fd == -1) {
        perror("epoll_create1");
        return LD_ERR_INTERNAL;
    }
    return LD_OK;
}

l_err register_stimer(stimer_cb cb, void *args, uint64_t nano) {

    pthread_t th;
    pthread_create(&th, NULL, stimer_event_dispatch, &(ld_stimer_handler_t){cb, args, nano});
    usleep(100);
    pthread_detach(th);
    return LD_OK;
}

l_err register_gtimer(ld_gtimer_handler_t *gtimer, int timer_tag, int64_t sec, int64_t nsec, int64_t wait_sec, int64_t wait_nsec) {

    timer_node_t *node = &gtimer->nodes[gtimer->node_count];
    node->cb_count = 0;
    node->timer_tag = timer_tag;

    if ((node->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0)) == -1) {
        log_error("timerfd_create");
        return LD_ERR_INTERNAL;
    }
    node->timer_spec.it_interval.tv_sec = sec;
    node->timer_spec.it_interval.tv_nsec = nsec;
    node->timer_spec.it_value.tv_sec = wait_sec;
    node->timer_spec.it_value.tv_nsec = wait_nsec == 0 ? 1 : wait_nsec;


    if (timerfd_settime(node->timer_fd, 0, &node->timer_spec, NULL) == -1) {
        perror("timerfd_settime");
        return 1;
    }

    node->event.events = EPOLLIN; // 监听可读事件
    node->event.data.fd = node->timer_fd;

    if (epoll_ctl(gtimer->epoll_fd, EPOLL_CTL_ADD, node->timer_fd, &node->event) == -1) {
        perror("epoll_ctl");
        return 1;
    }

    gtimer->node_count++;

    return LD_OK;
}

l_err register_gtimer_event(ld_gtimer_handler_t *gtimer, int timer_tag, gtimer_cb cb, void *args, uint64_t to_times) {
    for (int i = 0; i < gtimer->node_count; i++) {
        if (timer_tag == gtimer->nodes[i].timer_tag) {
            timer_node_t *node = &gtimer->nodes[i];
            node->cbs[node->cb_count] = (gtimer_cb_t){cb, args, to_times, 0};
            node->cb_count++;
            return LD_OK;
        }
    }
    return LD_ERR_NULL;
}

void start_gtimer(ld_gtimer_handler_t *gtimer) {
    pthread_create(&gtimer->th, NULL, gtimer_event_dispatch, gtimer);
    pthread_detach(gtimer->th);
}
