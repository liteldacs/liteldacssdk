//
// Created by jiaxv on 25-8-14.
//

#ifndef LD_EVENT_NET_H
#define LD_EVENT_NET_H

#include "ldacs_sim.h"
#include "ld_net.h"


#define MAX_LINE 1024

// 客户端连接结构体
typedef struct client_info {
    int fd;
    struct bufferevent *bev;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    int id;

    net_ctx_t *net_ctx;
} client_info_t;


struct role_propt2 {
    sock_roles s_r;

    int (*server_make)(uint16_t port, net_ctx_t *ctx);
};

const struct role_propt2 *get_role_propt2(int s_r);

l_err server_entity_setup2(uint16_t port, net_ctx_t *ctx, int s_r);

l_err default_send_pkt2(client_info_t *info, buffer_t *in_buf, l_err (*mid_func)(buffer_t *, void *),
                       void *args);

#endif //LD_EVENT_NET_H
