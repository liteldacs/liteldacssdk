//
// Created by jiaxv on 23-7-9.
//

#ifndef TEST_CLIENT_CLIENT_H
#define TEST_CLIENT_CLIENT_H
#include <ldacs_sim.h>
#include <netinet/tcp.h>
#include <ld_heap.h>
#include <ld_buffer.h>
#include <ld_mqueue.h>
#include <ld_epoll.h>
#include <passert.h>
#include <ld_santilizer.h>


#define IPV6_ADDRLEN 128
#define GEN_ADDRLEN 128
typedef struct basic_conn_s basic_conn_t;
# define DEFAULT_FD -1

struct role_propt {
    sock_roles s_r;

    int (*server_make)(uint16_t port);

    int (*handler)(basic_conn_t *);
};


typedef struct net_ctx_s {
    char name[32];
    int epoll_fd;
    int server_fd; //for GSW
    int timeout;
    heap_desc_t hd_conns;

    void (*close_handler)(basic_conn_t *);

    bool (*reset_conn)(basic_conn_t *);

    l_err (*recv_handler)(basic_conn_t *);

    // l_err (*send_handler)(basic_conn_t *, buffer_t *);
    l_err (*send_handler)(basic_conn_t *conn, void *pkg, struct_desc_t *desc, l_err (*mid_func)(buffer_t *, void *),
                     void *args);

    void *(*conn_handler)(struct net_ctx_s *ctx, char *remote_addr, int remote_port, int local_port);

    l_err (*accept_handler)(struct net_ctx_s *);
} net_ctx_t;

typedef struct basic_conn_s {
    int fd; /* connection_s fd */
    struct epoll_event event; /* epoll event */
    struct sockaddr_storage saddr; /* IP socket address */
    buffer_t read_pkt; /* Receive packet */
    lfqueue_t *write_pkts;
    bool trans_done;
    const struct role_propt *rp;
    struct net_ctx_s *opt;

    //client
    char *remote_addr;
    int remote_port;
    int local_port;
} basic_conn_t;

bool init_basic_conn(basic_conn_t *bc, net_ctx_t *ctx, sock_roles socket_role);

const struct role_propt *get_role_propt(int s_r);

void server_entity_setup(uint16_t port, net_ctx_t *opt);

void *client_entity_setup(net_ctx_t *opt, char *remote_addr, int remote_port, int local_port);


l_err defalut_send_pkt(basic_conn_t *bc, void *pkg, struct_desc_t *desc, l_err (*mid_func)(buffer_t *, void *),
                 void *args);

extern int request_handle(basic_conn_t *bc);

extern int response_handle(basic_conn_t *bc);

void *net_setup(void *args);

int net_epoll_add(int e_fd, basic_conn_t *conn_opt, uint32_t events, struct epoll_event *pev);

void net_epoll_out(int e_fd, basic_conn_t *bc);

void net_epoll_in(int e_fd, basic_conn_t *bc);

#endif //TEST_CLIENT_CLIENT_H
