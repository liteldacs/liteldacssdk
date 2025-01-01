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
}gtimer_cb_t;

typedef struct gtimer_node_s {
    int timer_fd;
    uint64_t timer_tag;
    struct itimerspec timer_spec;
    gtimer_cb_t cbs[10];
    size_t cb_count;
    struct epoll_event event;
} timer_node_t;

typedef struct ld_gtimer_handler_s {
    int epoll_fd;
    timer_node_t nodes[10];
    size_t node_count;
    pthread_t th;
} ld_gtimer_handler_t;

typedef struct ld_stimer_handler_s {
    stimer_cb cb;
    void *args;
    uint64_t nano;
} ld_stimer_handler_t;


l_err init_gtimer(ld_gtimer_handler_t *gtimer);


l_err register_gtimer(ld_gtimer_handler_t *gtimer, int timer_tag, int64_t sec, int64_t nsec, int64_t wait_sec,
                     int64_t wait_nsec);

l_err register_gtimer_event(ld_gtimer_handler_t *gtimer, int timer_tag, gtimer_cb cb, void *args, uint64_t to_times);

void start_gtimer(ld_gtimer_handler_t *gtimer);

l_err register_stimer(stimer_cb cb, void *args, uint64_t nano);


#endif //LD_NEWTIMER_H
