//
// Created by 邹嘉旭 on 2024/3/28.
//

#include "../global/ldacs_sim.h"
#include "ld_globaltimer.h"

ld_globaltimer_t g_timer;
ld_globaltimer_t c_timer;

void test_MF_func(void *args);

void test_SF_func_2(void *args);

ld_cycle_define_t cyc_def_s = {&g_timer, test_SF_func_2, NULL, TIMER_INFINITE, NULL};
ld_cycle_define_t cyc_def_m = {&c_timer, test_MF_func, NULL, 4, NULL};

void test_MF_func(void *args) {
    log_warn("MF!!!!");
}

void test_MF2_func(void *args) {
    log_warn("MF2!!!!");
}

void test_SF2_func(void *args) {
    log_debug("SF2!!!!");
}

// void *test_SF_func(void *args) {
//     log_debug("SF!!!!");
//     global_timer_t *child_timer = malloc(sizeof(global_timer_t));
//     init_global_timer(child_timer, timevs, 2);
//
//     register_timer_event(child_timer, 0, test_MF_func, NULL, 4);
//     start_global_timer(child_timer);
// }
//
//
// int main() {
//     global_timer_t *global_timer = malloc(sizeof(global_timer_t));
//     init_global_timer(global_timer, timevs, 2);
//
//     register_timer_event(global_timer, 1, test_SF_func, NULL, TIMER_INFINITE);
//     //register_timer_event(global_timer, 0, test_MF_func, NULL, 4);
//     start_global_timer(global_timer);
//
//     while(TRUE) {
//         sleep(100);
//     }
//     return 0;
// }

void test_SF_func_2(void *args) {
    log_debug("SF2!!!!");

    init_global_timer(&c_timer, MF_TIMER);
    register_timer_event(&cyc_def_m);
}


int main() {
    init_global_timer(&g_timer, SF_TIMER);
    register_timer_event(&cyc_def_s);

    while (TRUE) {
        sleep(1);
    }
    return 0;
}
