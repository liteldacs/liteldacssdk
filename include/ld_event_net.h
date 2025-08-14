//
// Created by jiaxv on 25-8-14.
//

#ifndef LD_EVENT_NET_H
#define LD_EVENT_NET_H

#include "ldacs_sim.h"
#include "ld_net.h"

struct role_propt2 {
    sock_roles s_r;

    int (*server_make)(uint16_t port, net_ctx_t *ctx);

    int (*handler)(basic_conn_t *);
};

l_err server_entity_setup2(uint16_t port, net_ctx_t *ctx, int s_r);

#endif //LD_EVENT_NET_H
