//
// Created by jiaxv on 23-9-2.
//

#include "ld_epoll.h"

struct epoll_event epoll_events[MAX_EVENTS];


int set_fd_nonblocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == ERROR) {
        log_error("fcntl F_GETFL failed: %s", strerror(errno));
        return ERROR;
    }
    flag |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) == ERROR) {
        log_error("fcntl F_SETFL failed: %s", strerror(errno));
        return ERROR;
    }
    return 0;
}


int core_epoll_create(int flags, int fd){
    if(fd == ERROR)     
        return epoll_create1(flags);
    else                
        return fd;
}


extern int core_epoll_wait(int epoll_fd, struct epoll_event *events,
                           int max_events, int timeout) {
    int result = epoll_wait(epoll_fd, events, max_events, timeout);
    if (result == ERROR && errno != EINTR) {
        log_error("epoll_wait failed: %s", strerror(errno));
    }
    return result;
}

int core_epoll_del(int epoll_fd, int fd, uint32_t events,struct epoll_event *pev) {
    (void)pev; /* Unused. Silent compiler warning. */
    int result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    if (result == ERROR) {
        log_warn("epoll_ctl DEL failed: %s", strerror(errno));
    }
    return result;
}

int core_epoll_add(int epoll_fd, int fd_p, struct epoll_event *pev) {
    int result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_p, pev);
    if (result == ERROR) {
        log_error("epoll_ctl ADD failed: %s", strerror(errno));
    }
    return result;
}