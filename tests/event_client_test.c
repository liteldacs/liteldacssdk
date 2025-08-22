// tcp_client.c - 基于libevent的高并发TCP客户端
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <event2/thread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define MAX_LINE 1024

// 客户端连接信息
typedef struct client_connection {
    int id;
    struct bufferevent *bev;
    struct event_base *base;
    int connected;
    pthread_t input_thread;
} client_connection_t;

// 全局变量
static volatile int should_exit = 0;
static client_connection_t *g_conn = NULL;

// 信号处理函数
static void signal_cb(evutil_socket_t sig, short events, void *user_data) {
    struct event_base *base = user_data;
    struct timeval delay = { 1, 0 };

    printf("\n捕获到信号 %d，准备退出...\n", sig);
    should_exit = 1;
    event_base_loopexit(base, &delay);
}

// 读事件回调函数
static void read_cb(struct bufferevent *bev, void *ctx) {
    client_connection_t *conn = (client_connection_t *)ctx;
    struct evbuffer *input = bufferevent_get_input(bev);
    char buffer[MAX_LINE];
    size_t len;

    // 读取所有可用数据
    while ((len = evbuffer_remove(input, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[len] = '\0';
        printf("\n[服务器消息]: %s", buffer);

        // 如果不是从终端输入，显示提示符
        if (!strstr(buffer, "\n")) {
            printf("\n");
        }
        printf("> ");
        fflush(stdout);
    }
}

// 写事件回调函数
static void write_cb(struct bufferevent *bev, void *ctx) {
    client_connection_t *conn = (client_connection_t *)ctx;
    struct evbuffer *output = bufferevent_get_output(bev);

    // 检查输出缓冲区是否已清空
    if (evbuffer_get_length(output) == 0) {
        // printf("数据发送完成\n");
    }
}

// 事件回调函数
static void event_cb(struct bufferevent *bev, short events, void *ctx) {
    client_connection_t *conn = (client_connection_t *)ctx;

    if (events & BEV_EVENT_CONNECTED) {
        printf("成功连接到服务器！\n");
        conn->connected = 1;
        printf("输入消息发送到服务器（输入'quit'退出）:\n> ");
        fflush(stdout);
    } else if (events & BEV_EVENT_EOF) {
        printf("服务器断开连接\n");
        conn->connected = 0;
        should_exit = 1;
    } else if (events & BEV_EVENT_ERROR) {
        printf("连接错误: %s\n", strerror(errno));
        conn->connected = 0;
        should_exit = 1;
    } else if (events & BEV_EVENT_TIMEOUT) {
        printf("连接超时\n");
        conn->connected = 0;
        should_exit = 1;
    }

    if (!conn->connected) {
        event_base_loopbreak(conn->base);
    }
}

// 用户输入线程函数
void *input_thread_func(void *arg) {
    client_connection_t *conn = (client_connection_t *)arg;
    char buffer[MAX_LINE];

    while (!should_exit && conn->connected) {
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            if (should_exit || !conn->connected) {
                break;
            }

            // 发送消息到服务器
            bufferevent_write(conn->bev, buffer, strlen(buffer));

            // 检查是否退出
            if (strncmp(buffer, "quit", 4) == 0) {
                should_exit = 1;
                event_base_loopbreak(conn->base);
                break;
            }

            printf("> ");
            fflush(stdout);
        }
    }

    return NULL;
}

// 创建单个客户端连接
client_connection_t *create_client_connection(struct event_base *base,
                                             const char *server_ip,
                                             int server_port,
                                             int client_id) {
    client_connection_t *conn = malloc(sizeof(client_connection_t));
    if (!conn) {
        return NULL;
    }

    conn->id = client_id;
    conn->base = base;
    conn->connected = 0;

    // 创建bufferevent
    conn->bev = bufferevent_socket_new(base, -1,
                                       BEV_OPT_CLOSE_ON_FREE);
    if (!conn->bev) {
        free(conn);
        return NULL;
    }

    // 设置回调函数
    bufferevent_setcb(conn->bev, read_cb, write_cb, event_cb, conn);

    // 启用读写事件
    bufferevent_enable(conn->bev, EV_READ | EV_WRITE);

    // 连接到服务器
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(server_ip);
    sin.sin_port = htons(server_port);

    printf("正在连接到服务器 %s:%d...\n", server_ip, server_port);

    if (bufferevent_socket_connect(conn->bev,
                                   (struct sockaddr *)&sin,
                                   sizeof(sin)) < 0) {
        bufferevent_free(conn->bev);
        free(conn);
        return NULL;
    }

    return conn;
}

// 压力测试：创建多个并发连接
void stress_test(struct event_base *base, int num_connections) {
    client_connection_t **connections = malloc(sizeof(client_connection_t*) * num_connections);
    char message[MAX_LINE];

    printf("开始压力测试：创建 %d 个并发连接...\n", num_connections);

    // 创建多个连接
    for (int i = 0; i < num_connections; i++) {
        connections[i] = create_client_connection(base, SERVER_IP, SERVER_PORT, i + 1);
        if (!connections[i]) {
            printf("创建连接 %d 失败\n", i + 1);
            continue;
        }

        // 每个连接发送一条消息
        snprintf(message, sizeof(message),
                "压力测试消息 - 来自客户端 %d\n", i + 1);
        bufferevent_write(connections[i]->bev, message, strlen(message));

        // 短暂延迟，避免瞬间创建过多连接
        usleep(10000);  // 10ms
    }

    printf("所有连接创建完成，进入事件循环...\n");

    // 运行5秒后退出
    struct timeval tv = {5, 0};
    event_base_loopexit(base, &tv);

    // 进入事件循环
    event_base_dispatch(base);

    // 清理资源
    for (int i = 0; i < num_connections; i++) {
        if (connections[i]) {
            if (connections[i]->bev) {
                bufferevent_free(connections[i]->bev);
            }
            free(connections[i]);
        }
    }
    free(connections);
}

int main(int argc, char *argv[]) {
    struct event_base *base = NULL;
    struct event *signal_event = NULL;
    const char *server_ip = SERVER_IP;
    int server_port = SERVER_PORT;
    int stress_mode = 0;
    int num_connections = 100;

    // 忽略SIGPIPE信号
    signal(SIGPIPE, SIG_IGN);

    // 解析命令行参数
    if (argc > 1) {
        if (strcmp(argv[1], "-s") == 0 || strcmp(argv[1], "--stress") == 0) {
            stress_mode = 1;
            if (argc > 2) {
                num_connections = atoi(argv[2]);
                if (num_connections <= 0) {
                    num_connections = 100;
                }
            }
        } else {
            server_ip = argv[1];
            if (argc > 2) {
                server_port = atoi(argv[2]);
            }
        }
    }

    // 初始化线程支持（如果使用多线程）
    evthread_use_pthreads();

    // 创建event_base
    base = event_base_new();
    if (!base) {
        fprintf(stderr, "创建event_base失败\n");
        return 1;
    }

    printf("使用的I/O方法: %s\n", event_base_get_method(base));

    if (stress_mode) {
        // 压力测试模式
        stress_test(base, num_connections);
    } else {
        // 普通客户端模式
        g_conn = create_client_connection(base, server_ip, server_port, 1);
        if (!g_conn) {
            fprintf(stderr, "创建客户端连接失败\n");
            event_base_free(base);
            return 1;
        }

        // 注册信号处理事件
        signal_event = evsignal_new(base, SIGINT, signal_cb, base);
        if (!signal_event || event_add(signal_event, NULL) < 0) {
            fprintf(stderr, "无法创建/添加信号事件\n");
            bufferevent_free(g_conn->bev);
            free(g_conn);
            event_base_free(base);
            return 1;
        }

        // 创建用户输入线程
        if (pthread_create(&g_conn->input_thread, NULL,
                          input_thread_func, g_conn) != 0) {
            fprintf(stderr, "创建输入线程失败\n");
            bufferevent_free(g_conn->bev);
            free(g_conn);
            event_base_free(base);
            return 1;
        }

        // 进入事件循环
        event_base_dispatch(base);

        // 等待输入线程结束
        pthread_cancel(g_conn->input_thread);
        pthread_join(g_conn->input_thread, NULL);

        // 清理资源
        if (signal_event) {
            event_free(signal_event);
        }
        if (g_conn) {
            if (g_conn->bev) {
                bufferevent_free(g_conn->bev);
            }
            free(g_conn);
        }
    }

    if (base) {
        event_base_free(base);
    }

    printf("客户端已退出\n");

    return 0;
}

/*
 * 编译说明：
 * gcc -o tcp_server tcp_server.c -levent -pthread
 * gcc -o tcp_client tcp_client.c -levent -pthread
 *
 * 运行说明：
 * 1. 启动服务器：
 *    ./tcp_server [端口号]
 *    例如: ./tcp_server 8888
 *
 * 2. 启动普通客户端：
 *    ./tcp_client [服务器IP] [端口号]
 *    例如: ./tcp_client 127.0.0.1 8888
 *
 * 3. 压力测试模式（创建多个并发连接）：
 *    ./tcp_client -s [连接数]
 *    例如: ./tcp_client -s 1000
 *
 * 依赖库安装：
 * Ubuntu/Debian: sudo apt-get install libevent-dev
 * CentOS/RHEL: sudo yum install libevent-devel
 * MacOS: brew install libevent
 */