//
// Created by 邹嘉旭 on 2024/12/30.
//

#ifndef LD_NEWTIMER_H
#define LD_NEWTIMER_H
#define MAX_TIMER 10
#include <stddef.h>
#include <stdint.h>
#include <bits/types/struct_itimerspec.h>
#include <sys/epoll.h>

#include <sys/timerfd.h>

#include "ld_multitimer.h"

#define SF_TIMER_TAG 0
#define MF_TIMER_TAG 1

typedef void *(*gtimer_cb)(void *);
typedef void (*ld_stimer_s)(evutil_socket_t fd, short event, void *arg);

typedef struct gtimer_ev_s {
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
    pthread_mutex_t mutex;
} ld_gtimer_handler_t;

typedef struct ld_stimer_handler_s {
    // stimer_ev_t *timer_ev;
    struct event_base *base;
    struct event *ev;
    pthread_t th;
    pthread_mutex_t mutex;
}ld_stimer_handler_t;

typedef struct stimer_ev_s {
    ld_stimer_s cb;
    void *args;
    uint64_t nano;
    ld_stimer_handler_t handler;
} stimer_ev_t;

typedef struct ld_gtimer_t {
    struct itimerspec spec;
    ld_gtimer_handler_t handler;
}ld_gtimer_t;


l_err register_gtimer(ld_gtimer_t *gtimer);

l_err unregister_gtimer(ld_gtimer_t *gtimer);

l_err reregister_gtimer(ld_gtimer_t *gtimer);

l_err register_gtimer_event(ld_gtimer_t *gtimer, gtimer_ev_t *timer_cb);

// ld_stimer_t *register_stimer(stimer_ev_t *timer_cb);
l_err register_stimer(stimer_ev_t *timer_cb);



#endif //LD_NEWTIMER_H
