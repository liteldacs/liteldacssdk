//
// Created by jiaxv on 23-9-2.
//

#include "ld_epoll.h"

struct epoll_event epoll_events[MAX_EVENTS];


int set_fd_nonblocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    ABORT_ON(flag == ERROR, "fcntl: F_GETFL");
    flag |= O_NONBLOCK;
    ABORT_ON(fcntl(fd, F_SETFL, flag) == ERROR, "fcntl: FSETFL");
    return 0;
}


int core_epoll_create(int flags, int fd){
    if(fd == ERROR)     return epoll_create1(flags);
    else                return fd;
}


extern int core_epoll_wait(int epoll_fd, struct epoll_event *events,
                           int max_events, int timeout) {
    return epoll_wait(epoll_fd, events, max_events, timeout);
}
int core_epoll_del(int epoll_fd, int fd, uint32_t events,struct epoll_event *pev) {
    (void)pev; /* Unused. Silent compiler warning. */
    return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

int core_epoll_add(int epoll_fd, int fd_p, struct epoll_event *pev) {
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_p, pev);
}