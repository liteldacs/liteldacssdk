// tcp_server.c - 基于libevent的高并发TCP服务端
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <event2/util.h>

#define PORT 8888
#define MAX_LINE 1024

// 客户端连接结构体
typedef struct client_info {
    int fd;
    struct bufferevent *bev;
    struct sockaddr_in addr;
    int id;
} client_info_t;

static int client_count = 0;
static int client_id_counter = 0;

// 信号处理函数
static void signal_cb(evutil_socket_t sig, short events, void *user_data) {
    struct event_base *base = user_data;
    struct timeval delay = { 1, 0 };

    printf("\n捕获到信号 %d，准备退出...\n", sig);
    event_base_loopexit(base, &delay);
}

// 读事件回调函数
static void read_cb(struct bufferevent *bev, void *ctx) {
    client_info_t *client = (client_info_t *)ctx;
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);

    char buffer[MAX_LINE];
    size_t len;

    // 读取所有可用数据
    while ((len = evbuffer_remove(input, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[len] = '\0';

        printf("[客户端 %d] 收到消息: %s", client->id, buffer);

        // 处理特殊命令
        if (strncmp(buffer, "quit", 4) == 0) {
            printf("[客户端 %d] 请求断开连接\n", client->id);
            bufferevent_free(bev);
            return;
        }

        // 构造响应消息
        char response[MAX_LINE];
        snprintf(response, sizeof(response),
                "[服务器响应] 已收到客户端%d的消息: %s",
                client->id, buffer);

        // 发送响应给客户端
        evbuffer_add(output, response, strlen(response));

        // 广播消息给所有其他客户端（可选功能）
        // broadcast_message(buffer, client->id);
    }
}

// 写事件回调函数
static void write_cb(struct bufferevent *bev, void *ctx) {
    client_info_t *client = (client_info_t *)ctx;
    struct evbuffer *output = bufferevent_get_output(bev);

    // 检查输出缓冲区是否已清空
    if (evbuffer_get_length(output) == 0) {
        printf("[客户端 %d] 数据发送完成\n", client->id);
    }
}

// 事件回调函数（处理错误和连接关闭）
static void event_cb(struct bufferevent *bev, short events, void *ctx) {
    client_info_t *client = (client_info_t *)ctx;

    if (events & BEV_EVENT_EOF) {
        printf("[客户端 %d] 连接关闭\n", client->id);
    } else if (events & BEV_EVENT_ERROR) {
        printf("[客户端 %d] 连接错误: %s\n",
               client->id, strerror(errno));
    } else if (events & BEV_EVENT_TIMEOUT) {
        printf("[客户端 %d] 连接超时\n", client->id);
    } else if (events & BEV_EVENT_CONNECTED) {
        printf("[客户端 %d] 连接建立成功\n", client->id);
        return;
    }

    // 清理资源
    bufferevent_free(bev);
    free(client);
    client_count--;
    printf("当前在线客户端数: %d\n", client_count);
}

// 接受新连接的回调函数
static void accept_cb(struct evconnlistener *listener,
                     evutil_socket_t fd,
                     struct sockaddr *address,
                     int socklen,
                     void *ctx) {
    struct event_base *base = ctx;
    struct bufferevent *bev;
    struct sockaddr_in *sin = (struct sockaddr_in *)address;

    // 创建客户端信息结构
    client_info_t *client = malloc(sizeof(client_info_t));
    if (!client) {
        printf("分配客户端结构失败\n");
        close(fd);
        return;
    }

    client->fd = fd;
    client->addr = *sin;
    client->id = ++client_id_counter;

    printf("[客户端 %d] 新连接来自 %s:%d\n",
           client->id,
           inet_ntoa(sin->sin_addr),
           ntohs(sin->sin_port));

    // 设置socket为非阻塞模式
    evutil_make_socket_nonblocking(fd);

    // 创建bufferevent
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        printf("创建bufferevent失败\n");
        free(client);
        close(fd);
        return;
    }

    client->bev = bev;

    // 设置回调函数
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, client);

    // 设置水位标记（可选）
    // bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);

    // 设置超时（可选，单位：秒）
    // struct timeval tv = {300, 0};  // 5分钟超时
    // bufferevent_set_timeouts(bev, &tv, &tv);

    // 启用读写事件
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    client_count++;
    printf("当前在线客户端数: %d\n", client_count);

    // 发送欢迎消息
    const char *welcome = "欢迎连接到高并发TCP服务器！\n";
    bufferevent_write(bev, welcome, strlen(welcome));
}

// 监听错误回调
static void accept_error_cb(struct evconnlistener *listener, void *ctx) {
    struct event_base *base = ctx;
    int err = EVUTIL_SOCKET_ERROR();

    fprintf(stderr, "监听器错误 %d: %s\n", err,
            evutil_socket_error_to_string(err));

    event_base_loopexit(base, NULL);
}

int main(int argc, char *argv[]) {
    struct event_base *base = NULL;
    struct evconnlistener *listener = NULL;
    struct event *signal_event = NULL;
    struct sockaddr_in sin;
    int port = PORT;

    // 忽略SIGPIPE信号
    signal(SIGPIPE, SIG_IGN);

    // 解析命令行参数
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            printf("无效端口号，使用默认端口 %d\n", PORT);
            port = PORT;
        }
    }

    // 创建event_base
    base = event_base_new();
    if (!base) {
        fprintf(stderr, "创建event_base失败\n");
        return 1;
    }

    // 输出使用的后端方法
    printf("使用的I/O方法: %s\n", event_base_get_method(base));

    // 设置监听地址
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);

    // 创建监听器
    listener = evconnlistener_new_bind(base, accept_cb, base,
                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
                                       -1,  // backlog，-1表示使用默认值
                                       (struct sockaddr *)&sin,
                                       sizeof(sin));

    if (!listener) {
        fprintf(stderr, "创建监听器失败\n");
        event_base_free(base);
        return 1;
    }

    // 设置监听错误回调
    evconnlistener_set_error_cb(listener, accept_error_cb);

    // 注册信号处理事件（SIGINT）
    signal_event = evsignal_new(base, SIGINT, signal_cb, base);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        fprintf(stderr, "无法创建/添加信号事件\n");
        evconnlistener_free(listener);
        event_base_free(base);
        return 1;
    }

    printf("TCP服务器启动成功，监听端口 %d\n", port);
    printf("按 Ctrl+C 退出服务器\n");
    printf("========================================\n");

    // 进入事件循环
    event_base_dispatch(base);

    // 清理资源
    printf("服务器正在关闭...\n");

    if (signal_event) {
        event_free(signal_event);
    }
    if (listener) {
        evconnlistener_free(listener);
    }
    if (base) {
        event_base_free(base);
    }

    printf("服务器已关闭\n");

    return 0;
}