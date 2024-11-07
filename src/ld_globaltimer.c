//
// Created by 邹嘉旭 on 2024/3/28.
//

#include "ld_globaltimer.h"


static void unregister_timer_event(ld_cycle_define_t *cyc_def);

static l_err init_sem_set(ld_bitset_t *bitset) {
    bitset->resources = calloc(bitset->res_num, bitset->res_sz);
    return LD_OK;
}

static void free_sem_set(void *res) {
    free(res);
}

static void ld_sem_init(ld_sem_t *l_sem, uint32_t val) {
    l_sem->is_destory = FALSE;
    sem_init(&l_sem->sem, 0, val);
}

static void ld_sem_wait(ld_sem_t *l_sem) {
    sem_wait(&l_sem->sem);
}

static void ld_sem_post(ld_sem_t *l_sem) {
    sem_post(&l_sem->sem);
}

static void ld_sem_destory(ld_sem_t *l_sem) {
    if (l_sem) {
        l_sem->is_destory = TRUE;
        sem_destroy(&l_sem->sem);
        memset(l_sem, 0, sizeof(ld_sem_t));
    }
}

static bool ld_sem_isdestory(const ld_sem_t *l_sem) {
    return l_sem->is_destory;
}

static int ld_sem_getvalue(ld_sem_t *l_sem) {
    int val = 0;
    sem_getvalue(&l_sem->sem, &val);
    return val;
}

/**
 * 持续分发事件
 * @param args
 * @return
 */
static void *timer_thread_func(void *args) {
    ld_globaltimer_t *g_timer = args;
    ld_lock(&g_timer->mutex);

    event_base_dispatch(g_timer->ev_base);


    event_base_free(g_timer->ev_base);
    ld_unlock(&g_timer->mutex);
    free_bitset(g_timer->timer_slot->l_sems_set);
    free(g_timer->timer_slot);
    /* 重置global timer */
    zero(g_timer);
    return NULL;
}

/**
 * 异步取消事件分发
 * @param g_timer
 */
static void timer_stop(ld_globaltimer_t *g_timer) {
    event_base_loopbreak(g_timer->ev_base);
}

/**
 * 根据纳秒计算微妙
 * @param timev
 * @param nano_time
 */
static void nano_to_timeval(struct timeval *timev, const uint64_t nano_time) {
    timev->tv_sec = nano_time / SECOND;
    timev->tv_usec = nano_time / MICROSECOND;
}

/**
 * 定时器事件，每隔一定事件把所有信号激活一次
 * @param fd
 * @param flags
 * @param args
 */
static void slot_activate(evutil_socket_t fd, short flags, void *args) {
    timer_slots_t *slot = args;

    ld_lock(&slot->mutex);
    for (int i = 0; i < TIMER_NUM_MAX; i++) {
        ld_sem_t *l_sem = RES_INDEX(slot->l_sems_set, i);

        if (bs_judge_resource(slot->l_sems_set, i) == TRUE) {
            //如果未达到临界状态，则不发送
            if (ld_sem_getvalue(l_sem) == 0) {
                ld_sem_post(l_sem);
            }
        }
    }
    ld_unlock(&slot->mutex);
}

static uint64_t get_microtime() {
    struct timeval curr_nano;
    if (gettimeofday(&curr_nano, NULL) != 0) {
        fprintf(stderr, "gettimeofday failed!\n");
        return -1;
    }
    return curr_nano.tv_sec * 1000000L + curr_nano.tv_usec; // 转换为微秒
}

/**
 * 初始化定时信号事件槽
 * @param base
 * @param nano
 */
static timer_slots_t *init_timer_slot(struct event_base *base, uint64_t nano) {
    timer_slots_t *slot = malloc(sizeof(timer_slots_t));
    if ((slot->l_sems_set = init_bitset(TIMER_NUM_MAX, sizeof(ld_sem_t), init_sem_set, free_sem_set)) == NULL) {
        free(slot);
        return NULL;
    }
    nano_to_timeval(&slot->tv, nano);
    pthread_mutex_init(&slot->mutex, NULL);

    event_set(&slot->ev, -1, EV_PERSIST, slot_activate, slot);
    event_base_set(base, &slot->ev);
    event_add(&slot->ev, &slot->tv);

    return slot;
}


l_err init_global_timer(ld_globaltimer_t *g_timer, const uint64_t time_val, const uint64_t sync_micro) {

    evthread_use_pthreads();
    ld_lock(&g_timer->mutex);
    g_timer->ev_base = event_base_new();
    ld_unlock(&g_timer->mutex);
    if ((g_timer->timer_slot = init_timer_slot(g_timer->ev_base, time_val)) == NULL) {
        return LD_ERR_INTERNAL;
    }


    if (sync_micro != 0) {
        uint64_t curr_time = get_microtime();
        uint64_t remainder = curr_time % sync_micro;
        uint64_t to_sync =  curr_time + (sync_micro-remainder);
        while (get_microtime() < to_sync) {}
    }

    pthread_create(&g_timer->th, NULL, timer_thread_func, g_timer);
    return LD_OK;
}

static void *wait_sem_func(void *args) {
    ld_cycle_define_t *cyc_def = args;
    int64_t handle_times = cyc_def->to_times;

    while (handle_times == TIMER_INFINITE || handle_times--) {
        //等待信号量
        if (handle_times == TIMER_INFINITE || cyc_def->is_instant == FALSE || handle_times != cyc_def->to_times - 1) {
            if (cyc_def->l_sem) {
                ld_sem_wait(cyc_def->l_sem);
            }
        }

        cyc_def->func(cyc_def->args);
    }

    //释放副本内存
    unregister_timer_event(cyc_def);

    return NULL;
}

/**
 * 注册信号量，并持续监听该信号量
 * @return
 */
void register_timer_event(ld_cycle_define_t *cyc_def) {
    timer_slots_t *slot = cyc_def->timer->timer_slot;

    ld_lock(&slot->mutex);

    //在副本中存储sem数组中所分配的对应位置，并初始化
    if (bs_alloc_resource(slot->l_sems_set, (void **) &cyc_def->l_sem) == LD_ERR_INVALID) return;

    ld_sem_init(cyc_def->l_sem, 0);

    ld_unlock(&slot->mutex);

    pthread_create(&cyc_def->th, NULL, wait_sem_func, cyc_def);
}

static void unregister_timer_event(ld_cycle_define_t *cyc_def) {
    timer_slots_t *slot = cyc_def->timer->timer_slot;
    // ld_lock(&slot->mutex);

    //slot的sem数组中的对应位置置空
    ld_sem_destory(cyc_def->l_sem);
    bs_free_resource(slot->l_sems_set, cyc_def->l_sem - (ld_sem_t *) slot->l_sems_set->resources);

    // ld_unlock(&slot->mutex);
    cyc_def->l_sem = NULL;

    if (bs_all_alloced(slot->l_sems_set) == TRUE) {
        timer_stop(cyc_def->timer);
    }

    pthread_cancel(cyc_def->th);
}
