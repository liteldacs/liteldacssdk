// //
// // Created by 邹嘉旭 on 2024/3/27.
// //
// #include <sys/types.h>
// #include <event2/event-ld_config.h>
// #include <event2/listener.h>
// #include <stdio.h>
// #include <event.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <pthread.h>
// #include <string.h>
// #include <signal.h>
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <fcntl.h>
// #include <ld_statemachine.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <semaphore.h>
//
// #define _DEBUG_INFO
// #ifdef _DEBUG_INFO
// #define DEBUG_INFO(format, ...) printf("%s:%d $$ "format"\n",__func__,__LINE__ , ##__VA_ARGS__)
// #else
// #define DEBUG_INFO(format, ...)
// #endif
//
// struct private_timer_data {
//     struct sm_event_s ev;
//     struct timeval tv;
// };
//
// void timer_1s_cb(int fd, short event, void *arg) {
//     struct private_timer_data *ptd = (struct private_timer_data *) arg;
//     DEBUG_INFO("fd = %d, event = %d", fd, (int)event);
//     DEBUG_INFO("event = %d,0x%02x %s %s %s %s", event, event,
//                ((event & EV_TIMEOUT)?"EV_TIMEOUT":""),
//                ((event & EV_READ)?"EV_READ":""),
//                ((event & EV_WRITE)?"EV_WRITE":""),
//                ((event & EV_PERSIST)?"EV_PERSIST":"")
//     );
//     static int count = 0;
//     count++;
//     if (count >= 10) {
//         event_del(&ptd->ev);
//         free(ptd);
//     }
// }
//
// void timer_5s_cb(int fd, short event, void *arg) {
//     struct private_timer_data *ptd = (struct private_timer_data *) arg;
//     DEBUG_INFO("fd = %d, event = %d", fd, (int)event);
//     DEBUG_INFO("event = %d,0x%02x %s %s %s %s", event, event,
//                ((event & EV_TIMEOUT)?"EV_TIMEOUT":""),
//                ((event & EV_READ)?"EV_READ":""),
//                ((event & EV_WRITE)?"EV_WRITE":""),
//                ((event & EV_PERSIST)?"EV_PERSIST":"")
//     );
//     static int count = 0;
//     count++;
//     if (count >= 10) {
//         event_del(&ptd->ev);
//         free(ptd);
//     }
// }
//
// void *base_01_thread(void *arg) {
//     struct event_base *base = (struct event_base *) arg;
//     while (event_base_get_num_events(base,EVENT_BASE_COUNT_ADDED) == 0) {
//         usleep(1000);
//     }
//
//     DEBUG_INFO("%d %d %d",
//                event_base_get_num_events(base,EVENT_BASE_COUNT_ADDED),
//                event_base_get_num_events(base,EVENT_BASE_COUNT_VIRTUAL),
//                event_base_get_num_events(base,EVENT_BASE_COUNT_ACTIVE));
//     event_base_dispatch(base);
//     event_base_free(base);
//     DEBUG_INFO("集合下班了");
//     return NULL;
// }
//
// int main(int argc, char *argv[]) {
//     struct event_base *base;
//     DEBUG_INFO("libevent version = %s", event_get_version());
//     base = event_base_new();
//     if (base == NULL) {
//         DEBUG_INFO("event_base_new error");
//     };
//     pthread_t __attribute__((unused)) t1;
//     pthread_t __attribute__((unused)) t2;
//
//     if (pthread_create(&t1,NULL, base_01_thread, base) < 0) {
//         perror((const char *) "pthread_create");
//         exit(-1);
//     }
//     struct private_timer_data *ptd;
//     //设置1秒定时器
//     ptd = (struct private_timer_data *) malloc(sizeof(struct private_timer_data));
//     ptd->tv.tv_sec = 1;
//     ptd->tv.tv_usec = 0;
//     // evtimer_set(&ptd->ev,timer_1s_cb,ptd);
//     event_set((&ptd->ev), -1, EV_PERSIST, (timer_1s_cb), (ptd));
//     event_base_set(base, &ptd->ev);
//     event_add(&ptd->ev, &ptd->tv);
//
//     //设置5秒定时器
//     ptd = (struct private_timer_data *) malloc(sizeof(struct private_timer_data));
//     ptd->tv.tv_sec = 3;
//     ptd->tv.tv_usec = 0;
//     // evtimer_set(&ptd->ev,timer_5s_cb,ptd);
//     event_set((&ptd->ev), -1, EV_PERSIST, (timer_5s_cb), (ptd));
//     event_base_set(base, &ptd->ev);
//     event_add(&ptd->ev, &ptd->tv);
//
//
//     DEBUG_INFO("%d %d %d",
//                event_base_get_num_events(base,EVENT_BASE_COUNT_ADDED),
//                event_base_get_num_events(base,EVENT_BASE_COUNT_VIRTUAL),
//                event_base_get_num_events(base,EVENT_BASE_COUNT_ACTIVE));
//     while (event_base_get_num_events(base,EVENT_BASE_COUNT_ADDED) > 0) {
//         sleep(1);
//     }
//     sleep(1);
//     DEBUG_INFO("byebye");
//     return 0;
// }
//
//
// int signal_count = 0;
//
// //fd这里是SIGINT 的数值，events这里是EV_SIGNAL | EV_PERSIST
// void signal_handler(evutil_socket_t fd, short events, void *arg) {
//     struct sm_event_s *ev = (struct sm_event_s *) arg;
//     printf("收到信号 %d\n", fd);
//
//     signal_count++;
//     //if(signal_count >= 2)
//     //{
//     //    //把事件从集合中删除
//     //    event_del(ev);
//     //}
// }
//
// void *cb(void *args) {
//     for (;;) {
//         sleep(1);
//         raise(SIGALRM);
//     }
// }
//
// int main() {
//     //创建事件集合
//     struct event_base *base = event_base_new();
//     //创建事件
//     struct sm_event_s ev;
//     sem_t mutex;
//     sem_init(&mutex, 0, 10086);
//     //把事件和信号绑定
//     event_assign(&ev, base, SIGALRM, EV_SIGNAL | EV_PERSIST, signal_handler, &ev);
//     //把事件添加到集合中
//     event_add(&ev, NULL);
//
//     pthread_t th;
//     pthread_create(&th, NULL, cb, NULL);
//
//     //监听集合
//     event_base_dispatch(base);
//
//     //释放集合
//     event_base_free(base);
//
//     return 0;
// }
int main() {
}
