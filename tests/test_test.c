//
// Created by 邹嘉旭 on 2024/11/7.
//
#include "ld_multitimer.h"
void callback_func(ld_multitimer_t *mt, single_timer_t *timer, void *args) {
    log_warn("TIMED OUT! %i", timer->id);
}

void callback_func_2(ld_multitimer_t *mt, single_timer_t *timer, void *args) {
    {
        //mt_chilog(INFO, mt, true);
        struct timeval tp;
        gettimeofday(&tp, NULL);
        log_info("Seconds:%ld\n", tp.tv_sec); // 微妙
        log_info("Microseconds:%ld\n", tp.tv_usec); // 微妙
        log_info("!!!!!!!!!\n");
    }
}

int main(int argc, char *argv[]) {

    ld_multitimer_t mt;
    mt_init(&mt, 5);

    // mt_set_timer_name(&mt, 0, "Retransmission");
    // mt_set_timer_name(&mt, 1, "Persist");
    // mt_set_timer_name(&mt, 2, "Delayed ACK");
    // mt_set_timer_name(&mt, 3, "2MSL");

    log_info("ACTIVE TIMERS");

    log_info("Setting all timers except for 2MSL...");
    mt_set_timer(&mt, 0, SECOND * 2, callback_func, NULL);
    mt_set_timer(&mt, 1, SECOND * 0.5, callback_func, NULL);
    mt_set_timer(&mt, 2, SECOND * 5, callback_func, NULL);

    sleep(3);
    log_info("Finish sleep");
    mt_set_timer(&mt, 0, SECOND * 1, callback_func, NULL);
    // bool is_stop = FALSE;
    //
    // //cycle_event_t *event = init_cycele_event(&mt, 4, callback_func_2, SECOND);
    // cycle_event_t event1 = {
    //         .mt = &mt,
    //         .timer_idx = 4,
    //         .times = 2,
    //         .stop_flag = &is_stop,
    //         .timer_func = callback_func_2,
    //         .time_intvl = SECOND,
    // };
    // //start_cycle_task(event);
    // start_cycle_task(&event1);
    //
    sleep(10);

    mt_free(&mt);

    // printf("%d\n", bytes_len(17));
    // printf("%lu", generate_urand(12));

    return 0;
}
