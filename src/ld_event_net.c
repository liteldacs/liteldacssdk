//
// Created by jiaxv on 25-8-14.
//

#include "ld_event_net.h"




static void default_tcp_accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address,
                                  int socklen, void *ctx);

static void default_tcpv6_accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address,
                                  int socklen, void *ctx);



l_err default_send_pkt2(client_info_t *info, buffer_t *in_buf, l_err (*mid_func)(buffer_t *, void *),
                       void *args) {
    if (!info) {
        log_error("Client info is null");
        return LD_ERR_NULL;
    }

    struct evbuffer *output = bufferevent_get_output(info->bev);
    buffer_t *buf = init_buffer_unptr();
    if (mid_func) {
        mid_func(buf, args);
    }
    cat_to_buffer(buf, in_buf->ptr, in_buf->len);
    // log_buf(LOG_INFO, "Send OUT", buf->ptr, buf->len);

    evbuffer_add(output, buf->ptr, buf->len);

    return LD_OK;
}

// 读事件回调函数
void default_read_cb(struct bufferevent *bev, void *ctx) {
    client_info_t *client = ctx;
    struct evbuffer *input = bufferevent_get_input(bev);

    char buffer[MAX_LINE];
    size_t len;

    // 读取所有可用数据
    while ((len = evbuffer_remove(input, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[len] = '\0';

        buffer_t *read_buf = init_buffer_unptr();
        CLONE_TO_CHUNK(*read_buf, buffer, len);
        log_buf(LOG_INFO, "Read", read_buf->ptr, read_buf->len);
        client->net_ctx->recv_handler2(read_buf);


        // // 构造响应消息
        // char response[MAX_LINE];
        // snprintf(response, sizeof(response),
        //         "[服务器响应] 已收到客户端%d的消息: %s",
        //         client->id, buffer);
        //
        // // 发送响应给客户端
        // evbuffer_add(output, response, strlen(response));

    }
}

// 写事件回调函数
static void default_write_cb(struct bufferevent *bev, void *ctx) {
    client_info_t *client = (client_info_t *)ctx;
    struct evbuffer *output = bufferevent_get_output(bev);

    // 检查输出缓冲区是否已清空
    if (evbuffer_get_length(output) == 0) {
        log_info("Send Finish");
    }
}

// 事件回调函数（处理错误和连接关闭）
static void default_event_cb(struct bufferevent *bev, short events, void *ctx) {
    client_info_t *client = (client_info_t *)ctx;
    if (!client) {
        bufferevent_free(bev);
        log_warn("Client NUll");
        return;
    }

    if (events & BEV_EVENT_EOF) {
        log_warn("client %d closed", client->id);
    } else if (events & BEV_EVENT_ERROR) {
        log_error("client %d connect fatal: %s",
               client->id, strerror(errno));
    } else if (events & BEV_EVENT_TIMEOUT) {
        log_warn("client %d time out", client->id);
    } else if (events & BEV_EVENT_CONNECTED) {
        log_info("client %d connect successful", client->id);
        return;
    }

    // 清理资源
    bufferevent_free(bev);
    free(client);
    client->net_ctx->client_count--;
    log_info("Current client: %d", client->net_ctx->client_count);
}

// 监听错误回调
static void default_accept_error_cb(struct evconnlistener *listener, void *ctx) {
    struct event_base *base = ctx;
    int err = EVUTIL_SOCKET_ERROR();

    fprintf(stderr, "监听器错误 %d: %s\n", err,
            evutil_socket_error_to_string(err));

    event_base_loopexit(base, NULL);
}

// 信号处理函数
static void default_signal_cb(evutil_socket_t sig, short events, void *user_data) {
    struct event_base *base = user_data;
    struct timeval delay = { 1, 0 };

    log_fatal("Catch signal %d，exiting...", sig);
    event_base_loopexit(base, &delay);
}

static l_err init_std_tcp_server_handler(uint16_t port, net_ctx_t *ctx) {
    memset(&ctx->sin, 0, sizeof(ctx->sin));
    ctx->sin.sin_family = AF_INET;
    ctx->sin.sin_addr.s_addr = htonl(INADDR_ANY);
    ctx->sin.sin_port = htons(port);
    ctx->listener = evconnlistener_new_bind(ctx->base, default_tcp_accept_cb, ctx,
                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
                                       -1,  // backlog，-1表示使用默认值
                                       (struct sockaddr *)&ctx->sin,
                                       sizeof(ctx->sin));

    if (!ctx->listener) {
        log_error("create listener failed");
        event_base_free(ctx->base);
        return LD_ERR_INTERNAL;
    }

    evconnlistener_set_error_cb(ctx->listener, default_accept_error_cb);
    return LD_OK;
}

static l_err init_std_tcpv6_server_handler(uint16_t port, net_ctx_t *ctx) {
    memset(&ctx->sin6, 0, sizeof(ctx->sin6));
    ctx->sin6.sin6_family = AF_INET6;
    ctx->sin6.sin6_addr = in6addr_any;
    ctx->sin6.sin6_port = htons(port);
    ctx->listener = evconnlistener_new_bind(ctx->base, default_tcpv6_accept_cb, ctx,
                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
                                       -1,  // backlog，-1表示使用默认值
                                       (struct sockaddr *)&ctx->sin6,
                                       sizeof(ctx->sin6));

    if (!ctx->listener) {
        log_error("create listener failed");
        event_base_free(ctx->base);
        return LD_ERR_INTERNAL;
    }

    evconnlistener_set_error_cb(ctx->listener, default_accept_error_cb);
    return LD_OK;
}

const struct role_propt2 role_propts[] = {
    {LD_TCP_CLIENT, NULL},
    {LD_TCPV6_CLIENT, NULL},
    {LD_TCP_SERVER, init_std_tcp_server_handler},
    {LD_TCPV6_SERVER, init_std_tcpv6_server_handler},
    {0, 0},
};

const struct role_propt2 *get_role_propt2(int s_r) {
    for (int i = 0; role_propts[i].s_r != 0; i++) {
        if (role_propts[i].s_r == s_r)
            return role_propts + i;
    }
    return NULL;
}



l_err server_entity_setup2(uint16_t port, net_ctx_t *ctx, int s_r) {
    ctx->base = event_base_new();
    if (!ctx->base) {
        log_error(stderr, "init event_base failed");
        return LD_ERR_INTERNAL;
    }

    log_info("Server using I/O method: %s", event_base_get_method(ctx->base));

    const struct role_propt2 *rp = get_role_propt2(s_r);
    if (rp->server_make) {
        ctx->server_fd = rp->server_make(port, ctx);
    }

    // 注册信号处理事件（SIGINT）
    ctx->signal_event = evsignal_new(ctx->base, SIGINT, default_signal_cb, ctx->base);
    if (!ctx->signal_event || event_add(ctx->signal_event, NULL) < 0) {
        log_error("can not add signal event.");
        evconnlistener_free(ctx->listener);
        event_base_free(ctx->base);
        return LD_ERR_INTERNAL;
    }

    log_info("TCP server start successfully, listening on port: %d.", port);
    // 进入事件循环
    event_base_dispatch(ctx->base);

    // 清理资源
    log_info("server closing...");

    if (ctx->signal_event) {
        event_free(ctx->signal_event);
    }
    if (ctx->listener) {
        evconnlistener_free(ctx->listener);
    }
    if (ctx->base) {
        event_base_free(ctx->base);
    }

    log_info("server closed");
    return LD_OK;
}



static void default_tcp_accept_cb(struct evconnlistener *listener,
                     evutil_socket_t fd,
                     struct sockaddr *address,
                     int socklen,
                     void *ctx) {
    net_ctx_t *net_ctx = ctx;
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
    client->id = ++ net_ctx->client_id_counter;
    client->net_ctx = net_ctx;

    printf("[客户端 %d] 新连接来自 %s:%d\n",
           client->id,
           inet_ntoa(sin->sin_addr),
           ntohs(sin->sin_port));

    // 设置socket为非阻塞模式
    evutil_make_socket_nonblocking(fd);

    // 创建bufferevent
    client->bev = bufferevent_socket_new(net_ctx->base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!client->bev) {
        printf("创建bufferevent失败\n");
        free(client);
        close(fd);
        return;
    }


    // 设置回调函数
    bufferevent_setcb(client->bev, default_read_cb, default_write_cb, default_event_cb, client);

    // 设置水位标记（可选）
    // bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);

    // 设置超时（可选，单位：秒）
    // struct timeval tv = {300, 0};  // 5分钟超时
    // bufferevent_set_timeouts(bev, &tv, &tv);

    // 启用读写事件
    bufferevent_enable(client->bev, EV_READ | EV_WRITE);

    net_ctx->client_count++;
    printf("当前在线客户端数: %d\n", net_ctx->client_count);

    // // 发送欢迎消息
    // const char *welcome = "欢迎连接到高并发TCP服务器！\n";
    // bufferevent_write(bev, welcome, strlen(welcome));
}

static void default_tcpv6_accept_cb(struct evconnlistener *listener,
                     evutil_socket_t fd,
                     struct sockaddr *address,
                     int socklen,
                     void *ctx) {
    net_ctx_t *net_ctx = ctx;
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)address;
    char ip_str[INET6_ADDRSTRLEN];

    // 创建客户端信息结构
    client_info_t *client = malloc(sizeof(client_info_t));
    if (!client) {
        printf("分配客户端结构失败\n");
        close(fd);
        return;
    }

    client->fd = fd;
    client->addr6 = *sin6;
    client->id = ++ net_ctx->client_id_counter;
    client->net_ctx = net_ctx;

    inet_ntop(AF_INET6, &sin6->sin6_addr, ip_str, INET6_ADDRSTRLEN);
    printf("[客户端 %d] 新连接来自 %s:%d\n",
           client->id,
           ip_str,
           ntohs(sin6->sin6_port));

    // 设置socket为非阻塞模式
    evutil_make_socket_nonblocking(fd);

    // 创建bufferevent
    client->bev = bufferevent_socket_new(net_ctx->base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!client->bev) {
        printf("创建bufferevent失败\n");
        free(client);
        close(fd);
        return;
    }


    // 设置回调函数
    bufferevent_setcb(client->bev, default_read_cb, default_write_cb, default_event_cb, client);

    // 设置水位标记（可选）
    // bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);

    // 设置超时（可选，单位：秒）
    // struct timeval tv = {300, 0};  // 5分钟超时
    // bufferevent_set_timeouts(bev, &tv, &tv);

    // 启用读写事件
    bufferevent_enable(client->bev, EV_READ | EV_WRITE);

    net_ctx->client_count++;
    printf("当前在线客户端数: %d\n", net_ctx->client_count);

    // // 发送欢迎消息
    // const char *welcome = "欢迎连接到高并发TCP服务器！\n";
    // bufferevent_write(bev, welcome, strlen(welcome));
}
