//
// Created by jiaxv on 23-8-2.
//

//TODO: 未来应使用libevetnt对定时器进行重写
#include "ld_multitimer.h"
/* args for multitimer thread function */

static int loglevel = ERROR;

struct worker_args {
    multi_timer_t *mt;
};

/* function for multitimer thread */
void *multiple_timer_machine(void *args);

/**
 * @brief helper function to compare two timespec
 *
 * @param timer_x
 * @param timer_y
 * @return int -1: x < y
 *             0 : x = y
 *             1 : x > y
 */
static int timespec_cmp(single_timer_t *timer_x, single_timer_t *timer_y) {
    struct timespec *x = &timer_x->expire_time;
    struct timespec *y = &timer_y->expire_time;
    if (x->tv_sec < y->tv_sec) {
        return -1;
    } else if (x->tv_sec > y->tv_sec) {
        return 1;
    }
    return x->tv_nsec - y->tv_nsec;
}

/**
 * @brief update expire time of the timer
 *
 * @param expire_time: pointer to the timespec struct of the timer
 * @param timeout: in nanoseconds
 */
void update_expire_time(struct timespec *expire_time, uint64_t timeout) {
    long secs = timeout / (long) 1e9;
    long nsec = timeout % (long) 1e9;
    clock_gettime(CLOCK_REALTIME, expire_time);
    long temp = expire_time->tv_nsec + nsec;
    expire_time->tv_sec += secs + temp / (long) 1e9;
    expire_time->tv_nsec = temp % (long) 1e9;
}

/* See ld_multitimer.h */
int timespec_subtract(struct timespec *result, struct timespec *x, struct timespec *y) {
    struct timespec tmp;
    tmp.tv_sec = y->tv_sec;
    tmp.tv_nsec = y->tv_nsec;

    /* Perform the carry for the later subtraction by updating tmp. */
    if (x->tv_nsec < tmp.tv_nsec) {
        uint64_t sec = (tmp.tv_nsec - x->tv_nsec) / SECOND + 1;
        tmp.tv_nsec -= SECOND * sec;
        tmp.tv_sec += sec;
    }
    if (x->tv_nsec - tmp.tv_nsec > SECOND) {
        uint64_t sec = (x->tv_nsec - tmp.tv_nsec) / SECOND;
        tmp.tv_nsec += SECOND * sec;
        tmp.tv_sec -= sec;
    }

    /* Compute the time remaining to wait.
       tv_nsec is certainly positive. */
    fprintf(stderr, "%ld %ld\n", x->tv_sec, tmp.tv_sec);
    fprintf(stderr, "%ld %ld\n", x->tv_nsec, tmp.tv_nsec);
    result->tv_sec = x->tv_sec - tmp.tv_sec;
    result->tv_nsec = x->tv_nsec - tmp.tv_nsec;

    /* Return 1 if result is negative. */
    return x->tv_sec < tmp.tv_sec;
}


/* See ld_multitimer.h */
l_err mt_init(multi_timer_t *mt, uint16_t num_timers) {
    mt->timer_num = num_timers;

    mt->all_timers = calloc(num_timers, sizeof(single_timer_t));
    if (mt->all_timers == NULL) {
        return LD_ERR_NOMEM;
    }
    uint16_t id = 0;
    for (uint16_t i = 0; i < num_timers; i++) {
        single_timer_t *cur = &mt->all_timers[i];
        cur->id = id++;
        cur->active = FALSE;
    }

    mt->active_timers = NULL;
    pthread_mutex_init(&mt->lock, NULL);
    pthread_cond_init(&mt->condvar, NULL);

    /* create multi-timer thread */
    struct worker_args *wa = calloc(1, sizeof(struct worker_args));
    wa->mt = mt;
    if (pthread_create(&mt->multiple_timer_thread, NULL, multiple_timer_machine, wa) != 0) {
        log_error("Could not create a multitimer thread");
        free(wa);
        pthread_exit(NULL);
        return LD_ERR_THREAD;
    }
    return LD_OK;
}

/* See ld_multitimer.h */
l_err mt_free(multi_timer_t *mt) {
    if (mt) {
        // stop the timer thread
        pthread_cancel(mt->multiple_timer_thread);
        // free related memory
        pthread_cond_destroy(&mt->condvar);
        pthread_mutex_destroy(&mt->lock);
        free(mt->all_timers);
    }
    return LD_OK;
}

/* See ld_multitimer.h */
l_err mt_get_timer_by_id(multi_timer_t *mt, uint16_t id, single_timer_t **timer) {
    if (id < 0 || id >= mt->timer_num) {
        return LD_ERR_INVALID;
    }
    *timer = &mt->all_timers[id];
    return LD_OK;
}

/* See ld_multitimer.h */
l_err mt_set_timer(multi_timer_t *mt, uint16_t id, uint64_t timeout, mt_callback_func callback, void *callback_args) {
    single_timer_t *timer;

    if (mt_get_timer_by_id(mt, id, &timer) == LD_ERR_INVALID || timer->active) {
        return LD_ERR_INVALID;
    }

    pthread_mutex_lock(&mt->lock);
    timer->active = TRUE;
    timer->callback = callback;
    timer->callback_args = callback_args;
    update_expire_time(&timer->expire_time, timeout);
    LL_INSERT_INORDER(mt->active_timers, timer, timespec_cmp);
    pthread_mutex_unlock(&mt->lock);
    pthread_cond_signal(&mt->condvar);
    return LD_OK;
}

/* See ld_multitimer.h */
l_err mt_cancel_timer(multi_timer_t *mt, uint16_t id) {
    single_timer_t *timer;
    if (mt_get_timer_by_id(mt, id, &timer) == LD_ERR_INVALID || !timer->active) {
        return LD_ERR_INTERNAL;
    }

    pthread_mutex_lock(&mt->lock);
    timer->active = FALSE;
    LL_DELETE(mt->active_timers, timer);
    timer->next = NULL;
    pthread_cond_signal(&mt->condvar);
    pthread_mutex_unlock(&mt->lock);

    return LD_OK;
}

/* See ld_multitimer.h */
l_err mt_set_timer_name(multi_timer_t *mt, uint16_t id, const char *name) {
    /* Your code here */
    single_timer_t *timer;
    if (mt_get_timer_by_id(mt, id, &timer) == LD_ERR_INVALID || !timer->active) {
        return LD_ERR_INTERNAL;
    }

    pthread_mutex_lock(&mt->lock);
    strncpy(timer->name, name, strlen(name));
    pthread_mutex_unlock(&mt->lock);
    return LD_OK;
}

/* mt_chilog_single_timer - Prints a single timer using chilog
 *
 * level: chilog log level
 *
 * timer: Timer
 *
 * Returns: Always returns CHITCP_OK
 */
l_err mt_chilog_single_timer(single_timer_t *timer) {
    struct timespec now, diff;
    clock_gettime(CLOCK_REALTIME, &now);

    if (timer->active) {
        /* Compute the appropriate value for "diff" here; it should contain
         * the time remaining until the timer times out.
         * Note: The timespec_subtract function can come in handy here*/
        struct timespec *res;
        timespec_subtract(res, &timer->expire_time, &now);
        diff.tv_sec = res->tv_sec;
        diff.tv_nsec = res->tv_nsec;
        log_info("%i %s %lis %lins", timer->id, timer->name, diff.tv_sec, diff.tv_nsec);
    } else
        log_info("%i %s", timer->id, timer->name);

    return LD_OK;
}

/* See ld_multitimer.h */
l_err mt_chilog(multi_timer_t *mt, bool active_only) {
    if (active_only) {
        pthread_mutex_lock(&mt->lock);
        single_timer_t *el;
        LL_FOREACH(mt->active_timers, el) {
            mt_chilog_single_timer(el);
        }
        pthread_mutex_unlock(&mt->lock);
    } else {
        for (int i = 0; i < mt->timer_num; i++) {
            mt_chilog_single_timer(&mt->all_timers[i]);
        }
    }
    return LD_OK;
}

void *multiple_timer_machine(void *args) {
    struct worker_args *wa = (struct worker_args *) args;
    multi_timer_t *mt = wa->mt;

    while (TRUE) {
        pthread_mutex_lock(&mt->lock);
        if (!mt->active_timers) {
            pthread_cond_wait(&mt->condvar, &mt->lock);
        } else {
            single_timer_t *first_timer = mt->active_timers;
            int rv = pthread_cond_timedwait(&mt->condvar, &mt->lock, &first_timer->expire_time);
            if (rv == ETIMEDOUT) {
                first_timer->num_timeouts++;
                first_timer->callback(mt, first_timer, first_timer->callback_args);
                first_timer->active = FALSE;
                LL_DELETE(mt->active_timers, first_timer);
                first_timer->next = NULL;
            }
        }
        pthread_mutex_unlock(&mt->lock);
    }
}

int get_active_num(multi_timer_t *mt) {
    int res = 0;
    for (int i = 0; i < mt->timer_num; i++) {
        if (mt->all_timers[i].active) {
            res++;
        }
    }
    return res;
}

static void *cycle_thread_func(void *args) {
    cycle_event_t *cycle_th = args;
    struct timespec sleepnano;

    sleepnano.tv_sec = (long) cycle_th->time_intvl / SECOND;
    sleepnano.tv_nsec = (long) cycle_th->time_intvl % SECOND;

    uint64_t to_times = cycle_th->times;

    while (*cycle_th->stop_flag != TRUE) {
        if (mt_set_timer(cycle_th->mt, cycle_th->timer_idx, 0, cycle_th->timer_func, args)) {
            log_error("Cannot set timmer by %d", cycle_th->timer_idx);
            break;
        }
        if (cycle_th->times == TIMER_INFINITE || --to_times) {
            nanosleep(&sleepnano, NULL);
            continue;
        } else {
            break;
        }
    }
}

cycle_event_t *init_cycele_event(multi_timer_t *mt, uint16_t idx,
                                 mt_callback_func cb_func, uint64_t intvl) {
    cycle_event_t *ev = malloc(sizeof(cycle_event_t));
    ev->mt = mt;
    ev->timer_idx = idx;
    ev->timer_func = cb_func;
    ev->time_intvl = intvl;
    return ev;
}

int start_cycle_task(cycle_event_t *c) {
    if (pthread_create(&c->th, NULL, cycle_thread_func, c) != 0) {
        pthread_exit(NULL);
    }
}
