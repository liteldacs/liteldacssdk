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
            gtimer_node_t *node = &gtimer->nodes;
            if (events[i].data.fd == node->timer_fd) {
                ssize_t ret = read(node->timer_fd, &expirations, sizeof(expirations));
                if (ret != sizeof(expirations)) {
                    perror("read");
                    return NULL;
                }

                for (int k = 0; k < node->cb_count; k++) {
                    gtimer_ev_t *cbt = &node->cbs[k];
                    if (cbt->to_times == TIMER_INFINITE || cbt->has_times < cbt->to_times) {
                        pthread_create(&cbt->th, NULL, cbt->cb, cbt->args);
                        pthread_detach(cbt->th);
                        cbt->has_times++;
                    }
                }
            }

            bool all_finished = TRUE;
            for (int j = 0; j < node->cb_count; j++) {
                if (node->cbs[j].to_times == TIMER_INFINITE || node->cbs[j].has_times != node->cbs[j].to_times) {
                    all_finished = FALSE;
                    break;
                }
            }
            if (all_finished == TRUE) {
                return NULL;
            }
        }
    }
    return NULL;
}

static void *stimer_event_dispatch(void *arg) {
    stimer_ev_t *stimer = calloc(1, sizeof(stimer_ev_t));
    stimer->cb = ((stimer_ev_t *)arg)->cb;
    stimer->args = ((stimer_ev_t *)arg)->args;
    stimer->nano = ((stimer_ev_t *)arg)->nano;

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

static l_err init_gtimer_node(ld_gtimer_handler_t *gtimer, struct itimerspec *spec) {
    gtimer_node_t *node = &gtimer->nodes;
    node->cb_count = 0;

    if ((node->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0)) == -1) {
        log_error("timerfd_create");
        return LD_ERR_INTERNAL;
    }

    if (timerfd_settime(node->timer_fd, 0, spec, NULL) == -1) {
        perror("timerfd_settime");
        return 1;
    }

    node->event.events = EPOLLIN; // 监听可读事件
    node->event.data.fd = node->timer_fd;

    if (epoll_ctl(gtimer->epoll_fd, EPOLL_CTL_ADD, node->timer_fd, &node->event) == -1) {
        perror("epoll_ctl");
        return 1;
    }

    return LD_OK;
}


ld_gtimer_handler_t *init_gtimer_handler(struct itimerspec *spec) {
    ld_gtimer_handler_t *ghandler = calloc(1, sizeof(ld_gtimer_handler_t));

    // 创建 epoll 实例
    ghandler->epoll_fd = epoll_create1(0);
    if (ghandler->epoll_fd == -1) {
        perror("epoll_create1");
        return NULL;
    }

    init_gtimer_node(ghandler, spec);
    return ghandler;
}

l_err register_stimer(stimer_ev_t *timer_cb) {

    pthread_t th;
    pthread_create(&th, NULL, stimer_event_dispatch, timer_cb);
    pthread_detach(th);
    return LD_OK;
}


l_err register_gtimer_event(ld_gtimer_t *gtimer, gtimer_ev_t *timer_cb) {
    if (gtimer->handler == NULL)    return LD_ERR_NULL;

    gtimer_node_t *node = &gtimer->handler->nodes;
    timer_cb->has_times = 0;
    node->cbs[node->cb_count] = *timer_cb;
    node->cb_count++;
    return LD_OK;
}

static void *start_gtimer(void *args) {
    ld_gtimer_t *gtimer = args;

    gtimer_event_dispatch(gtimer->handler);
    free(gtimer->handler);
    gtimer->handler = NULL;
    return NULL;
}

l_err register_gtimer(ld_gtimer_t *gtimer) {
    if (gtimer->spec.it_value.tv_nsec == 0) {
        gtimer->spec.it_value.tv_nsec =1;
    }
    gtimer->handler = init_gtimer_handler(&gtimer->spec);

    pthread_create(&gtimer->th, NULL, start_gtimer, gtimer);
    pthread_detach(gtimer->th);

    return LD_OK;
}