//
// Created by 邹嘉旭 on 2024/12/29.
//
#include <ld_newtimer.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>        /* Definition of uint64_t */
#include <utils/ld_log.h>

void *mf_event(void *args);
void *sf_event(void *args);
void *mf_event2(void *args);
void singal_event(evutil_socket_t fd, short event, void *arg);

static gtimer_ev_t sf_cb = {sf_event, NULL, TIMER_INFINITE};
static gtimer_ev_t mf_cb = {mf_event, NULL, 4};
static gtimer_ev_t mf_cb2 = {mf_event2, NULL, 4};
static stimer_ev_t mf_singal_cb = {singal_event, NULL, 40000000};
static ld_gtimer_t sf_global_cb = {{.it_interval = {0, 240000000}, .it_value = {0, 0}}};
static ld_gtimer_t mf_global_cb = {{.it_interval = {0, 58000000}, .it_value = {0, 0}}};


void singal_event(evutil_socket_t fd, short event, void *arg) {
    log_warn("++++++++");
}

void *mf_event(void *args) {
    log_warn("!!! MF EVENT");
    register_stimer(&mf_singal_cb);
    return NULL;
}


void *sf_event(void *args) {
    log_warn("!!! SF EVENT");

    register_gtimer(&mf_global_cb);
    register_gtimer_event(&mf_global_cb, &mf_cb);
    // register_gtimer_event(&mf_global_cb, &mf_cb2);
    return NULL;
}

void *mf_event2(void *args) {
    log_error("!!! MF EVENT2");
    return NULL;
}

int main() {
    log_init(LOG_DEBUG,  "../../log", "test");
    register_gtimer(&sf_global_cb);
    register_gtimer_event(&sf_global_cb, &sf_cb);
    sleep(100000);
}

