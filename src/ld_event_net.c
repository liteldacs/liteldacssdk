//
// Created by jiaxv on 25-8-14.
//

#include "ld_event_net.h"

static l_err init_std_tcp_server_handler(uint16_t port, net_ctx_t *ctx) {
    memset(&ctx->sin, 0, sizeof(ctx->sin));
    ctx->sin.sin_family = AF_INET;
    ctx->sin.sin_addr.s_addr = htonl(INADDR_ANY);
    ctx->sin.sin_port = htons(port);
    ctx->listener = evconnlistener_new_bind(ctx->base, ctx->accept_cb, ctx,
                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
                                       -1,  // backlog，-1表示使用默认值
                                       (struct sockaddr *)&ctx->sin,
                                       sizeof(ctx->sin));

    if (!ctx->listener) {
        log_error("create listener failed");
        event_base_free(ctx->base);
        return LD_ERR_INTERNAL;
    }

    evconnlistener_set_error_cb(ctx->listener, ctx->accept_error_cb);
    return LD_OK;
}

static l_err init_std_tcpv6_server_handler(uint16_t port, net_ctx_t *ctx) {
    memset(&ctx->sin6, 0, sizeof(ctx->sin6));
    ctx->sin6.sin6_family = AF_INET6;
    ctx->sin6.sin6_addr = in6addr_any;
    ctx->sin6.sin6_port = htons(port);
    ctx->listener = evconnlistener_new_bind(ctx->base, ctx->accept_cb, ctx,
                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
                                       -1,  // backlog，-1表示使用默认值
                                       (struct sockaddr *)&ctx->sin6,
                                       sizeof(ctx->sin6));

    if (!ctx->listener) {
        log_error("create listener failed");
        event_base_free(ctx->base);
        return LD_ERR_INTERNAL;
    }

    evconnlistener_set_error_cb(ctx->listener, ctx->accept_error_cb);
    return LD_OK;
}

const struct role_propt role_propts[] = {
    {LD_TCP_CLIENT, NULL, init_std_tcp_conn_handler},
    {LD_TCPV6_CLIENT, NULL, init_std_tcpv6_conn_handler},
    {LD_TCP_SERVER, init_std_tcp_server_handler, init_std_tcp_accept_handler},
    {LD_TCPV6_SERVER, init_std_tcpv6_server_handler, init_std_tcpv6_accept_handler},
    {0, 0, 0},
};

const struct role_propt *get_role_propt(int s_r) {
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

    const struct role_propt *rp = get_role_propt(s_r);
    if (rp->server_make) {
        ctx->server_fd = rp->server_make(port);
    }

    // 注册信号处理事件（SIGINT）
    ctx->signal_event = evsignal_new(ctx->base, SIGINT, ctx->signal_cb, ctx->base);
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

    log_info("server closed\n");
    return LD_OK;
}



// 客户端连接结构体
typedef struct client_info {
    int fd;
    struct bufferevent *bev;
    struct sockaddr_in addr;
    int id;
} client_info_t;

void default_tcp_accept_cb(struct evconnlistener *listener,
                     evutil_socket_t fd,
                     struct sockaddr *address,
                     int socklen,
                     void *ctx) {
    net_ctx_t *net_ctx = ctx;
    struct bufferevent *bev = NULL;
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

    printf("[客户端 %d] 新连接来自 %s:%d\n",
           client->id,
           inet_ntoa(sin->sin_addr),
           ntohs(sin->sin_port));

    // 设置socket为非阻塞模式
    evutil_make_socket_nonblocking(fd);

    // 创建bufferevent
    bev = bufferevent_socket_new(net_ctx->base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        printf("创建bufferevent失败\n");
        free(client);
        close(fd);
        return;
    }

    client->bev = bev;

    // 设置回调函数
    bufferevent_setcb(bev, net_ctx->read_cb, net_ctx->write_cb, net_ctx->event_cb, client);

    // 设置水位标记（可选）
    // bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);

    // 设置超时（可选，单位：秒）
    // struct timeval tv = {300, 0};  // 5分钟超时
    // bufferevent_set_timeouts(bev, &tv, &tv);

    // 启用读写事件
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    net_ctx->client_count++;
    printf("当前在线客户端数: %d\n", net_ctx->client_count);

    // // 发送欢迎消息
    // const char *welcome = "欢迎连接到高并发TCP服务器！\n";
    // bufferevent_write(bev, welcome, strlen(welcome));
}
