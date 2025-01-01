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
void singal_event(evutil_socket_t fd, short event, void *arg);

static gtimer_cb_t sf_cb = {sf_event, NULL, TIMER_INFINITE};
static gtimer_cb_t mf_cb = {mf_event, NULL, 4};
static stimer_cb_t mf_singal_cb = {singal_event, NULL, 40000000};


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
    ld_gtimer_handler_t ntimer;
    init_gtimer(&ntimer, 0, 60000000, 0, 0);
    // register_gtimer(&ntimer, 0, 60000000, 0, 0);
    // register_gtimer_event(&ntimer, MF_TIMER_TAG, mf_event, NULL, 4);
    register_gtimer_event(&ntimer, &mf_cb);

    start_gtimer(&ntimer);
    sleep(100000);
    return NULL;
}

void *mf_event2(void *args) {
    log_error("!!! MF EVENT2");
    return NULL;
}

int
main(int argc, char *argv[])
{
    log_init(LOG_DEBUG,  "../../log", "test");
    ld_gtimer_handler_t ntimer;
    init_gtimer(&ntimer, 0, 240000000, 0 , 0);

    // register_gtimer(&ntimer, 0, 240000000, 0, 0);
    // register_gtimer_event(&ntimer, SF_TIMER_TAG, sf_event, NULL, TIMER_INFINITE);
    register_gtimer_event(&ntimer, &sf_cb);
    start_gtimer(&ntimer);

    // sleep(3);
    // register_gtimer_event(&ntimer, MF_TIMER_TAG, mf_event2, NULL);
    sleep(10000);

    // for (int i = 0; i < 3; i++) {
    //     register_stimer(singal_event, NULL, SECOND);
    // }
    // sleep(10000);
}
