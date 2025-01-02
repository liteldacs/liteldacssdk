//
// Created by 邹嘉旭 on 2024/12/30.
//

#ifndef LD_NEWTIMER_H
#define LD_NEWTIMER_H
#define MAX_TIMER 10
#include <stddef.h>
#include <stdint.h>
#include <bits/types/struct_itimerspec.h>
#include <ldacs/global/ldacs_def.h>
#include <sys/epoll.h>

#include <ldacs/global/ldacs_def.h>
#include <ldacs/utils/ld_log.h>
#include <sys/timerfd.h>

#include "ld_multitimer.h"

#define SF_TIMER_TAG 0
#define MF_TIMER_TAG 1

typedef void *(*gtimer_cb)(void *);
typedef void (*stimer_cb)(evutil_socket_t fd, short event, void *arg);

typedef struct gtimer_cb_s {
    gtimer_cb cb;
    void *args;
    uint64_t to_times;
    uint64_t has_times;
    pthread_t th;
}gtimer_ev_t;

typedef struct gtimer_node_s {
    int timer_fd;
    gtimer_ev_t cbs[10];
    size_t cb_count;
    struct epoll_event event;
} gtimer_node_t;

typedef struct ld_gtimer_handler_s {
    int epoll_fd;
    gtimer_node_t nodes;
    pthread_t th;
} ld_gtimer_handler_t;

typedef struct ld_stimer_s {
    stimer_cb cb;
    void *args;
    uint64_t nano;
} stimer_ev_t;

typedef struct ld_gtimer_s {
    struct itimerspec spec;
    pthread_t th;
    ld_gtimer_handler_t *handler;
}ld_gtimer_t;


l_err register_gtimer(ld_gtimer_t *gtimer);

l_err register_gtimer_event(ld_gtimer_t *gtimer, gtimer_ev_t *timer_cb);

l_err register_stimer(stimer_ev_t *timer_cb);



#endif //LD_NEWTIMER_H
