//
// Created by jiaxv on 23-9-2.
//

#ifndef LDACS_SIM_UTIL_EPOLL_H
#define LDACS_SIM_UTIL_EPOLL_H

#include "../global/ldacs_sim.h"
#include "ld_log.h"

#define MAX_EVENTS (8192)

#define FILL_EPOLL_EVENT(pev, e_ptr, e_events)                                 \
  do {                                                                         \
    struct epoll_event *ev = pev;                                              \
    ev->data.ptr = e_ptr;                                                      \
    ev->events = e_events;                                                     \
  } while (0)


#define EPOLL_IS_IN(event) (event->events & EPOLLIN)
#define EPOLL_IS_OUT(event) (event->events & EPOLLOUT)


extern int epoll_fd;

extern struct epoll_event epoll_events[MAX_EVENTS]; // global

int set_fd_nonblocking(int fd);

int core_epoll_create(int flags, int fd);

extern int core_epoll_wait(int epoll_fd, struct epoll_event *events,
                           int max_events, int timeout);

int core_epoll_del(int epoll_fd, int fd, uint32_t events,
                   struct epoll_event *pev);

int core_epoll_add(int epoll_fd, int fd_p, struct epoll_event *pev);

static inline int epoll_enable_in(int e_fd, struct epoll_event *ev, int fd) {
    if (EPOLL_IS_IN(ev))
        return OK;
    ev->events |= EPOLLIN;
    return epoll_ctl(e_fd, EPOLL_CTL_MOD, fd, ev);
}

static inline int epoll_disable_in(int e_fd, struct epoll_event *ev, int fd) {
    if (!EPOLL_IS_IN(ev)) {
        return OK;
    }
    ev->events &= ~EPOLLIN;
    return epoll_ctl(e_fd, EPOLL_CTL_MOD, fd, ev);
}

static inline int epoll_enable_out(int e_fd, struct epoll_event *ev, int fd) {
    if (EPOLL_IS_OUT(ev)) {
        return OK;
    }
    ev->events |= EPOLLOUT;
    return epoll_ctl(e_fd, EPOLL_CTL_MOD, fd, ev);
}

static inline int epoll_disable_out(int e_fd, struct epoll_event *ev, int fd) {
    if (!EPOLL_IS_OUT(ev))
        return OK;
    ev->events &= ~EPOLLOUT;
    return epoll_ctl(e_fd, EPOLL_CTL_MOD, fd, ev);
}


#endif //LDACS_SIM_UTIL_EPOLL_H
