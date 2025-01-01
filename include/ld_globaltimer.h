//
// Created by 邹嘉旭 on 2024/3/28.
//

#ifndef LD_GLOBALTIMER_H
#define LD_GLOBALTIMER_H
#include "../global/ldacs_sim.h"
#include "ld_bitset.h"
#include "ld_thread.h"
#include "ld_util_def.h"
#include "ld_multitimer.h"

#define TIMER_NUM_MAX 128
#define TIMER_BIT_SET (TIMER_NUM_MAX / 8 + (TIMER_NUM_MAX % 8 > 0))


typedef struct ld_sem_s {
    bool is_destory;
    sem_t sem;
} ld_sem_t;

typedef struct timer_slots_s {
    struct event ev;
    struct timeval tv;
    // int8_t sem_size;

    ld_bitset_t *l_sems_set;

    // ld_sem_t l_sems[TIMER_NUM_MAX];
    // uint8_t l_sems_bitset[TIMER_BIT_SET]; // 位图数组

    pthread_mutex_t mutex;
} timer_slots_t;

typedef struct ld_globaltimer_s {
    struct event_base *ev_base;
    pthread_t th;
    pthread_mutex_t mutex;
    timer_slots_t *timer_slot;
} ld_globaltimer_t;


typedef struct ld_cycle_define_s {
    ld_globaltimer_t *timer;

    void (*func)(void *);

    void *args;
    int64_t to_times;
    is_stop volatile *stop_flag;
    bool is_instant;

    ld_sem_t *l_sem;
    pthread_t th;
    pthread_t func_th;
} ld_cycle_define_t;

l_err init_global_timer(ld_globaltimer_t *g_timer, const uint64_t time_val, const uint64_t sync_micro);

void register_timer_event(ld_cycle_define_t *cyc_def);

typedef struct ld_activetimer_s {
    struct event_base *ev_base;
} ld_activetimer_t;


#endif //LD_GLOBALTIMER_H
