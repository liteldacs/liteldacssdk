//
// Created by 邹嘉旭 on 2024/12/30.
//

#include "ld_newtimer.h"

#include <ld_thread.h>

static l_err update_gtimer_basetime(ld_gtimer_handler_t *ghandler, struct itimerspec *spec);
static l_err gtimer_timerfd_add(ld_gtimer_handler_t *handler);
static l_err gtimer_timerfd_del(ld_gtimer_handler_t *handler);

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
    ld_gtimer_handler_t *ghandler = args;
    while (1) {
        struct epoll_event events[10];
        int num_events = epoll_wait(ghandler->epoll_fd, events, 10, -1); // 阻塞等待事件
        if (num_events == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            return NULL;
        }

        uint64_t expirations;
        for (int i = 0; i < num_events; i++) {
            gtimer_node_t *node = &ghandler->nodes;
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
                    }else {
                    }
                }
            }

            bool all_finished = TRUE;
            if (node->cb_count == 0) {
                continue;
            }
            for (int j = 0; j < node->cb_count; j++) {
                // log_warn("%d  %d", node->cbs[j].has_times, node->cbs[j].to_times);
                if (node->cbs[j].to_times == TIMER_INFINITE || node->cbs[j].has_times != node->cbs[j].to_times) {
                    all_finished = FALSE;
                    break;
                }
            }
            if (all_finished == TRUE) {
                // if (epoll_ctl(ghandler->epoll_fd, EPOLL_CTL_DEL, node->timer_fd, NULL) == -1) {
                //     perror("epoll_ctl_del");
                //     return NULL;
                // }
                // close(node->timer_fd);
                gtimer_timerfd_del(ghandler);
                return NULL;
            }
        }
    }
    return NULL;
}

void stimer_fatal_log_cb(int severity, const char *msg) {
    log_error("[libevent] %s\n", msg);
}
static void *stimer_event_dispatch(void *arg) {
    stimer_ev_t *stimer= arg;
    ld_stimer_handler_t *handler = &stimer->handler;
    ld_lock(&handler->mutex);
    struct timeval tv;
    event_set_log_callback(stimer_fatal_log_cb);
    evthread_use_pthreads();
    handler->base = event_base_new();
    nano_to_timeval(&tv, stimer->nano);

    handler->ev = event_new(handler->base, -1, 0, stimer->cb, stimer->args);
    if (!handler->ev) {
        log_warn("Could not create event!\n");
        return NULL;
    }
    event_add(handler->ev, &tv);

    event_base_dispatch(handler->base);

    // 清理资源
    event_free(handler->ev);
    event_base_free(handler->base);
    handler->base = NULL;
    ld_unlock(&handler->mutex);
    return NULL;
}

static l_err init_gtimer_node(ld_gtimer_handler_t *ghandler, struct itimerspec *spec) {
    gtimer_node_t *node = &ghandler->nodes;
    node->cb_count = 0;

    if (update_gtimer_basetime(ghandler, spec) != LD_OK) {
        return LD_ERR_INTERNAL;
    }

    return LD_OK;
}

static l_err update_gtimer_basetime(ld_gtimer_handler_t *ghandler, struct itimerspec *spec) {
    gtimer_node_t *node = &ghandler->nodes;
    if ((node->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0)) == -1) {
        log_error("timerfd_create");
        return LD_ERR_INTERNAL;
    }

    if (timerfd_settime(node->timer_fd, 0, spec, NULL) == -1) {
        perror("timerfd_settime");
        return LD_ERR_INTERNAL;
    }

    node->event.events = EPOLLIN; // 监听可读事件
    node->event.data.fd = node->timer_fd;

    // if (epoll_ctl(ghandler->epoll_fd, EPOLL_CTL_ADD, node->timer_fd, &node->event) == -1) {
    //     perror("epoll_ctl_add");
    //     return LD_ERR_INTERNAL;
    // }
    gtimer_timerfd_add(ghandler);
    return LD_OK;
}


l_err init_gtimer_handler(struct itimerspec *spec, ld_gtimer_handler_t *ghandler) {
    // 创建 epoll 实例
    ghandler->epoll_fd = epoll_create1(0);
    if (ghandler->epoll_fd == -1) {
        perror("epoll_create1");
        return LD_ERR_NULL;
    }

    init_gtimer_node(ghandler, spec);
    return LD_OK;
}

l_err register_stimer(stimer_ev_t *stimer) {
    //
    // ld_stimer_t *stimer = calloc(1, sizeof(ld_stimer_t));
    // stimer->timer_ev = timer_cb;
    ld_stimer_handler_t *handler = &stimer->handler;

    pthread_create(&handler->th, NULL, stimer_event_dispatch, stimer);
    pthread_detach(handler->th);
    return LD_OK;
}


l_err register_gtimer_event(ld_gtimer_t *gtimer, gtimer_ev_t *timer_cb) {
    /* delay 1ms to assure the gtimer dispatch thread has started. */
    usleep(1000);

    gtimer_node_t *node = &gtimer->handler.nodes;
    timer_cb->has_times = 0;
    memcpy(&node->cbs[node->cb_count], timer_cb, sizeof(gtimer_ev_t));
    // node->cbs[node->cb_count] = *timer_cb;
    node->cb_count++;
    return LD_OK;
}

static void *start_gtimer(void *args) {
    ld_gtimer_t *gtimer = args;
    ld_gtimer_handler_t *handler = &gtimer->handler;
    ld_lock(&handler->mutex);

    gtimer_event_dispatch(handler);
    close(handler->epoll_fd);
    memset(handler, 0, sizeof(ld_gtimer_handler_t));
    ld_unlock(&handler->mutex);
    return NULL;
}

l_err register_gtimer(ld_gtimer_t *gtimer) {
    if (gtimer->spec.it_value.tv_nsec == 0) {
        gtimer->spec.it_value.tv_nsec =1;
    }
    ld_gtimer_handler_t *handler = &gtimer->handler;

    init_gtimer_handler(&gtimer->spec, handler);
    pthread_create(&handler->th, NULL, start_gtimer, gtimer);
    pthread_detach(handler->th);

    return LD_OK;
}

l_err unregister_gtimer(ld_gtimer_t *gtimer) {
    ld_gtimer_handler_t *handler = &gtimer->handler;
    if (handler->th == 0)   return LD_ERR_NULL;
    pthread_cancel(handler->th);
    close(handler->epoll_fd);
    memset(handler, 0, sizeof(ld_gtimer_handler_t));
    ld_unlock(&handler->mutex);
    return LD_OK;
}

l_err reregister_gtimer(ld_gtimer_t *gtimer) {
    ld_gtimer_handler_t *handler = &gtimer->handler;
    if (handler->th == 0)   return LD_ERR_NULL;
    pthread_cancel(handler->th);
    if (gtimer_timerfd_del(handler) != LD_OK || update_gtimer_basetime(handler, &gtimer->spec) != LD_OK) {
        return LD_ERR_INTERNAL;
    }
    ld_unlock(&handler->mutex);

    pthread_create(&handler->th, NULL, start_gtimer, gtimer);
    pthread_detach(handler->th);
    return LD_OK;
}

static l_err gtimer_timerfd_add(ld_gtimer_handler_t *handler) {
    if (epoll_ctl(handler->epoll_fd, EPOLL_CTL_ADD, handler->nodes.timer_fd, &handler->nodes.event) == -1) {
        perror("epoll_ctl_del");
        return LD_ERR_INTERNAL;
    }
    return LD_OK;
}

static l_err gtimer_timerfd_del(ld_gtimer_handler_t *handler) {
    if (epoll_ctl(handler->epoll_fd, EPOLL_CTL_DEL, handler->nodes.timer_fd, NULL) == -1) {
        perror("epoll_ctl_del");
        return LD_ERR_INTERNAL;
    }
    close(handler->nodes.timer_fd);
    return LD_OK;
}
