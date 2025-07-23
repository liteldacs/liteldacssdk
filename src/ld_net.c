//
// Created by jiaxv on 23-7-9.
//

#include "ld_net.h"

#define BACKLOG 1024
#define RECONNECT 20

static int server_shutdown(int server_fd);

static bool connecion_is_expired(basic_conn_t *bcp);

static int connection_register(basic_conn_t *bc, int64_t factor);

static void connection_set_nodelay(int fd);

static void connection_close(basic_conn_t *bc);

static void connecion_set_reactivated(basic_conn_t *bc);

static void connecion_set_expired(basic_conn_t *bcp);

static void server_connection_prune(net_ctx_t *opt);

static int make_std_tcp_connect(struct sockaddr_in *to_conn_addr, char *addr, int remote_port, int local_port) {
    struct in_addr s;
    int fd;


    inet_pton(AF_INET, addr, &s);
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
        return ERROR;

    int enable = SO_REUSEADDR;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    // struct timeval timeout = {
    //     .tv_sec = 5, /* after 5 seconds connect() will timeout  */
    //     .tv_usec = 0,
    // };
    //
    // setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    zero(to_conn_addr);
    to_conn_addr->sin_family = AF_INET;
    to_conn_addr->sin_port = htons(remote_port);
    // to_conn_addr->sin_addr = s;
    memcpy(&to_conn_addr->sin_addr, &s, sizeof(s));

    /* 绑定本地端口 */
    if (local_port != 0) {
        struct sockaddr_in local_addr;
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(local_port); // 转换为网络字节序
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 允许任意本地地址绑定

        if (bind(fd, (struct sockaddr *) &local_addr, sizeof(local_addr)) == -1) {
            perror("bind failed");
            close(fd);
            return -1;
        }
    }

    //TODO: 改成死循环，持续1min
    int i = RECONNECT;
    while (i--) {
        log_info("Trying to connect to remote `%s:%d` for %d time(s).", addr, remote_port, RECONNECT - i);
        if (connect(fd, (struct sockaddr *) to_conn_addr, sizeof(struct sockaddr_in)) >= 0) {
            log_info("Connected");
            return fd;
        }
        sleep(1);
    }

    log_error("Failed to connect. Exit...");

    return ERROR;
}

static int make_std_tcpv6_connect(struct sockaddr_in6 *to_conn_addr, char *addr, int remote_port, int local_port) {
    struct in6_addr s;
    int fd;


    inet_pton(AF_INET6, addr, &s);
    if ((fd = socket(AF_INET6, SOCK_STREAM, 0)) == ERROR)
        return ERROR;

    int enable = SO_REUSEADDR;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    // struct timeval timeout = {
    //     .tv_sec = 5, /* after 5 seconds connect() will timeout  */
    //     .tv_usec = 0,
    // };
    // setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    zero(to_conn_addr);
    to_conn_addr->sin6_family = AF_INET6;
    to_conn_addr->sin6_port = htons(remote_port);
    // to_conn_addr->sin_addr = s;
    memcpy(&to_conn_addr->sin6_addr, &s, sizeof(s));


    /* 绑定本地端口 */
    if (local_port != 0) {
        struct sockaddr_in6 local_addr;
        local_addr.sin6_family = AF_INET6;
        local_addr.sin6_port = htons(local_port); // 转换为网络字节序
        local_addr.sin6_addr = in6addr_any; // 允许任意本地地址绑定

        if (bind(fd, (struct sockaddr *) &local_addr, sizeof(local_addr)) == -1) {
            perror("bind failed");
            close(fd);
            return -1;
        }
    }

    //TODO: 改成死循环，持续1min
    int i = RECONNECT;
    while (i--) {
        log_info("Trying to connect to remote `%s:%d` for %d time(s).", addr, remote_port, RECONNECT - i);
        if (connect(fd, (struct sockaddr *) to_conn_addr, sizeof(struct sockaddr_in6)) >= 0) {
            log_info("Connected");
            return fd;
        }
        sleep(1);
    }

    log_error("Failed to connect. Exit...");

    return ERROR;
}

static int make_std_tcp_server(uint16_t port) {
    struct sockaddr_in saddr;
    int n_fd;

    if ((n_fd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
        return ERROR;

    int enable = SO_REUSEADDR;
    setsockopt(n_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    // if (config.worker > 1) {
    //     // since linux 3.9
    //     setsockopt(n_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
    // }

    zero(&saddr);
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(n_fd, (struct sockaddr *) &saddr, sizeof(saddr)) != OK)
        return ERROR;
    if (listen(n_fd, BACKLOG) != OK)
        return ERROR;

    return n_fd;
}

static int make_std_tcpv6_server(uint16_t port) {
    struct sockaddr_in6 saddr;
    int n_fd;

    if ((n_fd = socket(AF_INET6, SOCK_STREAM, 0)) == ERROR)
        return ERROR;

    int enable = 1; // SO_REUSEADDR and SO_REUSEPORT flag value

    setsockopt(n_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    // if (config.worker > 1) {
    //     // since linux 3.9
    //     setsockopt(n_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));
    // }

    // Allow both IPv6 and IPv4 connections on this socket.
    // If you want to restrict it to IPv6 only, set this option to 1.
    int v6only = 0;
    setsockopt(n_fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));

    memset(&saddr, 0, sizeof(saddr)); // Zero out the structure
    saddr.sin6_family = AF_INET6;
    saddr.sin6_port = htons(port);
    memcpy(&saddr.sin6_addr, &in6addr_any, sizeof(in6addr_any));

    if (bind(n_fd, (struct sockaddr *) &saddr, sizeof(saddr)) != OK) {
        return ERROR;
    }
    if (listen(n_fd, BACKLOG) != OK)
        return ERROR;

    return n_fd;
}

static int make_std_tcp_accept(basic_conn_t *bc) {
    struct sockaddr_in *to_conn_addr = (struct sockaddr_in *) &bc->saddr;
    int fd;
    socklen_t saddrlen = sizeof(struct sockaddr_in);
    if (bc->opt->server_fd == DEFAULT_FD) return DEFAULT_FD;
    while ((fd = accept(bc->opt->server_fd, (struct sockaddr *) to_conn_addr, &saddrlen)) == ERROR) {
    }
    return fd;
}

static int make_std_tcpv6_accept(basic_conn_t *bc) {
    struct sockaddr_in6 *to_conn_addr = (struct sockaddr_in6 *) &bc->saddr;
    int fd;
    socklen_t saddrlen = sizeof(struct sockaddr_in6);
    if (bc->opt->server_fd == DEFAULT_FD) return DEFAULT_FD;
    while ((fd = accept(bc->opt->server_fd, (struct sockaddr *) to_conn_addr, &saddrlen)) == ERROR) {

    }

    return fd;
}

static int init_std_tcp_conn_handler(basic_conn_t *bc) {
    return make_std_tcp_connect((struct sockaddr_in *) &bc->saddr, bc->remote_addr, bc->remote_port, bc->local_port);
}

static int init_std_tcpv6_conn_handler(basic_conn_t *bc) {
    return make_std_tcpv6_connect((struct sockaddr_in6 *) &bc->saddr, bc->remote_addr, bc->remote_port, bc->local_port);
}

static int init_std_tcp_accept_handler(basic_conn_t *bc) {
    return make_std_tcp_accept(bc);
}

static int init_std_tcpv6_accept_handler(basic_conn_t *bc) {
    return make_std_tcpv6_accept(bc);
}



const struct role_propt role_propts[] = {
    {LD_TCP_CLIENT, NULL, init_std_tcp_conn_handler},
    {LD_TCPV6_CLIENT, NULL, init_std_tcpv6_conn_handler},
    {LD_TCP_SERVER, make_std_tcp_server, init_std_tcp_accept_handler},
    {LD_TCPV6_SERVER, make_std_tcpv6_server, init_std_tcpv6_accept_handler},
    {0, 0, 0},
};

const struct role_propt *get_role_propt(int s_r) {
    for (int i = 0; role_propts[i].s_r != 0; i++) {
        if (role_propts[i].s_r == s_r)
            return role_propts + i;
    }
    return NULL;
}


static int add_listen_fd(int epoll_fd, int server_fd) {
    set_fd_nonblocking(server_fd);
    struct epoll_event ev;
    int *fd_ptr = calloc(1, sizeof(int));
    memcpy(fd_ptr, &server_fd, sizeof(int));
    ev.data.ptr = fd_ptr;
    ev.events = EPOLLIN | EPOLLET;
    return core_epoll_add(epoll_fd, server_fd, &ev);
}

void server_entity_setup(uint16_t port, net_ctx_t *opt, int s_r) {
    const struct role_propt *rp = get_role_propt(s_r);

    opt->server_fd = rp->server_make(port);

    ABORT_ON(opt->accept_handler == NULL, "Accept handler is NULL");
    ABORT_ON(opt->server_fd == ERROR, "make_server");
    ABORT_ON(add_listen_fd(opt->epoll_fd, opt->server_fd) == ERROR, "add_listen_fd");

    log_info("Server has started successfully.");
}

void *client_entity_setup(net_ctx_t *opt, char *remote_addr, int remote_port, int local_port) {
    void *conn = opt->conn_handler(opt, remote_addr, remote_port, local_port);
    if (!conn) return NULL;
    return conn;
}

int server_shutdown(int server_fd) {
    return close(server_fd);
}


l_err defalut_send_pkt(basic_conn_t *bc, void *pkg, struct_desc_t *desc, l_err (*mid_func)(buffer_t *, void *),
                 void *args) {
    if (bc == NULL) return LD_ERR_INTERNAL;
    pb_stream pbs;
    uint8_t raw[2048] = {0};


    init_pbs(&pbs, raw, 2048, "Send BUF");
    if (!out_struct(pkg, desc, &pbs, NULL)) {
        log_error("Cannot generate sending message!");
        return LD_ERR_INTERNAL;
    }

    close_output_pbs(&pbs);

    buffer_t *buf = init_buffer_unptr();
    if (mid_func) {
        mid_func(buf, args);
    }
    cat_to_buffer(buf, pbs.start, pbs_offset(&pbs));
    log_buf(LOG_INFO, "Send OUT", buf->ptr, buf->len);

    lfqueue_put(bc->write_pkts, buf);
    net_epoll_out(bc->opt->epoll_fd, bc);
    return LD_OK;
}

static int read_packet(int fd, basic_conn_t *bc) {
    // // 先读取4字节的长度头
    // uint32_t pkt_len;
    // ssize_t len = read(fd, &pkt_len, sizeof(pkt_len));
    // if (len != sizeof(pkt_len)) {
    //     if (len < 0) log_warn("Read header error");
    //     return len == 0 ? END : ERROR;
    // }
    // // 转换网络字节序到主机字节序
    // pkt_len = ntohl(pkt_len);
    //
    // log_warn("!!!!!!!! PKT READ LEN:  %d", pkt_len);
    //
    // uint8_t temp[pkt_len];
    // // buffer_t *buf = init_buffer_unptr();
    // len = read(fd, temp, pkt_len);
    //
    // if (len == pkt_len) {
    //     bc->read_pkt = init_buffer_unptr();
    //     CLONE_TO_CHUNK(*bc->read_pkt, temp, len);
    //     bc->opt->recv_handler(bc);
    //     free_buffer(bc->read_pkt);
    //     return OK;
    // } else {
    //     log_warn("Incomplete packet: %d/%u bytes", len, pkt_len);
    //     return ERROR;
    // }

    uint8_t temp[MAX_INPUT_BUFFER_SIZE] = {0};

    ssize_t len = read(fd, temp, sizeof(temp));
    // log_warn("!!!!! %d", len);
    if (len > 0) {
        uint8_t *cur = temp;
        while (TRUE) {
            uint32_t pkt_len;
            memcpy(&pkt_len, cur, sizeof(pkt_len));
            cur += sizeof(pkt_len);
            pkt_len = ntohl(pkt_len);
            // log_warn("!!!!!! RECV LEN %d", pkt_len);
            if (pkt_len > MAX_INPUT_BUFFER_SIZE)return ERROR;
            if (pkt_len == 0) break;

            log_buf(LOG_ERROR, "RECVV", cur, pkt_len);

            bc->read_pkt = init_buffer_unptr();
            CLONE_TO_CHUNK(*bc->read_pkt, cur, pkt_len);
            cur += pkt_len;

            if (bc->opt->recv_handler(bc) != LD_OK) {
                log_error("Cannot handler received message");
                return ERROR;
            }
            free_buffer(bc->read_pkt);
        }

        // CLONE_TO_CHUNK(bc->read_pkt, temp, len)
        return OK;
    } else {
        log_warn("Read from socket size: %d", len);
        return ERROR;
    }
}


/**
 * Return:
 * OK: all data sent
 * AGAIN: haven't sent all data
 * ERROR: error
 */
static int write_packet(basic_conn_t *bc) {

    while (lfqueue_size(bc->write_pkts) != 0) {
        buffer_t *b = NULL;
        buffer_t *to_send = init_buffer_unptr();
        lfqueue_get(bc->write_pkts, (void **) &b);
        if (!b) return ERROR;
        size_t len = b->len;
        // 添加4字节长度头
        uint32_t pkt_len = htonl(len);

        cat_to_buffer(to_send, (uint8_t *)&pkt_len, sizeof(pkt_len));
        cat_to_buffer(to_send, b->ptr, len);

        // log_warn("!!!!!!!! PKT LEN:  %d", len);
        // log_warn("!!!!!!!! PKT SEND LEN:  %d", to_send->len);

        //
        // // 先发送长度头
        // if (write(bc->fd, &pkt_len, sizeof(pkt_len)) != sizeof(pkt_len)) {
        //     return ERROR;
        // }
        // 发送实际数据
        size_t sent = 0;
        while (sent < to_send->len) {
            // ssize_t n = write(bc->fd, (char*)b->ptr + sent, len - sent);
            ssize_t n = write(bc->fd, (char*)to_send->ptr + sent, to_send->len - sent);
            if (n <= 0) {
                return n == 0 ? OK : ERROR;
            }
            sent += n;
        }

        free_buffer(b);

        // len = write(bc->fd, b->ptr, b->len);
        //
        // /* delay the next transmission */
        // // usleep(1000);
        //
        // if (!len) {
        //     return ERROR;
        // }
    }
    return OK;
}

int request_handle(basic_conn_t *bc) {
    if (bc->opt->recv_handler) {
        if (read_packet(bc->fd, bc) == ERROR) return ERROR;
        // bc->opt->recv_handler(bc);
    }

    return OK;
}


static int response_send_buffer(basic_conn_t *bc) {
    int status = write_packet(bc);
    // if (status != OK) {
    //     return status;
    // } else {
    //     bc->trans_done = TRUE;
    //     return OK;
    // }

    switch (status) {
        case OK:
            net_epoll_in(bc->opt->epoll_fd, bc);
            bc->trans_done = TRUE;
            break;

        case AGAIN:
            // 保持EPOLLOUT等待剩余数据
            break;

        case ERROR:
    bc->trans_done = TRUE;
            connecion_set_expired(bc);
            break;
    }

    return status;
}


int response_handle(basic_conn_t *bc) {
    int status;

    // if (bc->opt->send_handler) {
    //     bc->opt->send_handler(bc);
    // }
    do {
        status = response_send_buffer(bc);
    } while (status == OK && bc->trans_done != TRUE);
    if (bc->trans_done) {
        // response done
        if (bc->opt->reset_conn) bc->opt->reset_conn(bc);
        net_epoll_in(bc->opt->epoll_fd, bc);
    }
    return status;
}

void *net_setup(void *args) {
    int nfds;
    int i;
    net_ctx_t *net_ctx = args;

    while (TRUE) {
        nfds = core_epoll_wait(net_ctx->epoll_fd, epoll_events, MAX_EVENTS, -1);

        if (nfds == ERROR) {
            // if not caused by signal, cannot recover
            ERR_ON(errno != EINTR, "core_epoll_wait");
        }

        /* processing ready fd one by one */
        for (i = 0; i < nfds; i++) {
            struct epoll_event *curr_event = epoll_events + i;
            int fd = *((int *) curr_event->data.ptr);
            if (fd == net_ctx->server_fd) {
                // gs_conn_accept(net_opt); /* never happened in GS */
                net_ctx->accept_handler(net_ctx);
            } else {
                basic_conn_t *bc = curr_event->data.ptr;
                int status;
                assert(bc != NULL);

                if (connecion_is_expired(bc)) {
                    log_warn("Expired connection");
                    continue;
                }
                bool should_reactivate = false;
                bool has_error = false;

                if (curr_event->events & EPOLLIN) {
                    if (request_handle(bc) == OK) {
                        should_reactivate = true;
                    } else {
                        log_error("error when epoll in");
                        has_error = true;
                    }
                }
                if (curr_event->events & EPOLLOUT) {
                    if (response_handle(bc) == OK) {
                        should_reactivate = true;
                    } else {
                        log_error("error when epoll out");
                        has_error = true;
                    }
                }

                // if (has_error) {
                //     connecion_set_expired(bc);
                // } else if (should_reactivate) {
                //     connecion_set_reactivated(bc);
                // }

                    connecion_set_reactivated(bc);

                // if (curr_event->events & EPOLLIN) {
                //     //recv
                //     status = request_handle(bc);
                // }
                // if (curr_event->events & EPOLLOUT) {
                //     //send
                //     status = response_handle(bc);
                // }
                //
                // if (status == ERROR) {
                //     connecion_set_expired(bc);
                // } else {
                //     connecion_set_reactivated(bc);
                // }
            }
        }
        server_connection_prune(net_ctx);
    }
    close(net_ctx->epoll_fd);
    server_shutdown(net_ctx->server_fd);
}


int net_epoll_add(int e_fd, basic_conn_t *bc, uint32_t events,
                  struct epoll_event *pev) {
    FILL_EPOLL_EVENT(pev, bc, events);
    return core_epoll_add(e_fd, bc->fd, pev);
}

void net_epoll_out(int e_fd, basic_conn_t *bc) {
    epoll_disable_in(e_fd, &bc->event, bc->fd);
    epoll_enable_out(e_fd, &bc->event, bc->fd);
}

void net_epoll_in(int e_fd, basic_conn_t *bc) {
    epoll_disable_out(e_fd, &bc->event, bc->fd);
    epoll_enable_in(e_fd, &bc->event, bc->fd);
}


#define ADDR_LEN (64/BITS_PER_BYTE)

/**
 * Store the basic_conn_t addresses into the propts in large end mode
 * @param start the start address of propts struct
 * @param addr address of basic_conn_t
 */
static void set_basic_conn_addr(uint8_t *start, void *addr) {
    uint64_t addr_int = (uint64_t) addr;
    for (size_t i = 0; i < ADDR_LEN; i++) {
        start[i] = (uint8_t)(addr_int >> (BITS_PER_BYTE * i));
    }
}

bool init_basic_conn(basic_conn_t *bc, net_ctx_t *ctx, sock_roles socket_role) {
    do {
        bc->fd = 0;
        bc->opt = ctx;
        bc->rp = get_role_propt(socket_role);
        bc->fd = bc->rp->handler(bc);

        if (bc->fd == ERROR) break;

        ABORT_ON(bc->opt->epoll_fd == 0 || bc->opt->epoll_fd == ERROR, "illegal epoll fd");

        if (connection_register(bc, time(NULL)) == ERROR) break;
        set_fd_nonblocking(bc->fd);
        connection_set_nodelay(bc->fd);

        // zero(&bc->read_pkt);
        bc->write_pkts = lfqueue_init();

        net_epoll_add(bc->opt->epoll_fd, bc, EPOLLIN | EPOLLET, &bc->event);
        return TRUE;
    } while (0);

    connection_close(bc);
    return FALSE;
}

static void connection_set_nodelay(int fd) {
    static int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
}

bool connecion_is_expired(basic_conn_t *bc) {
    heap_t *conn_hp = get_heap(&bc->opt->hd_conns, bc);
    int64_t active_time = conn_hp->factor;
    return bc->opt->timeout ? (time(NULL) - active_time > bc->opt->timeout) : FALSE;
}

void connecion_set_reactivated(basic_conn_t *bc) {
    heap_t *conn_hp = get_heap(&bc->opt->hd_conns, bc);
    if (!conn_hp) return;
    conn_hp->factor = time(NULL); /* active_time */
    if (bc->rp->s_r & 1) heap_bubble_down(&bc->opt->hd_conns, conn_hp->heap_idx);
}

void connecion_set_expired(basic_conn_t *bc) {
    heap_t *conn_hp = get_heap(&bc->opt->hd_conns, bc);
    if (!conn_hp) return;
    conn_hp->factor = 0; // very old time
    if (bc->rp->s_r & 1) heap_bubble_up(&bc->opt->hd_conns, conn_hp->heap_idx);
    connection_close(bc);
}

static int connection_register(basic_conn_t *bc, int64_t factor) {
    if (bc->opt->hd_conns.heap_size >= MAX_HEAP) {
        return ERROR;
    }
    return heap_insert(&bc->opt->hd_conns, bc, factor);
}

void connection_unregister(basic_conn_t *bc) {
    assert(bc->opt->hd_conns.heap_size >= 1);

    heap_t *conn_hp = get_heap(&bc->opt->hd_conns, bc);
    int heap_idx = conn_hp->heap_idx;
    bc->opt->hd_conns.hps[heap_idx] = bc->opt->hd_conns.hps[bc->opt->hd_conns.heap_size - 1];
    bc->opt->hd_conns.hps[heap_idx]->heap_idx = heap_idx;
    bc->opt->hd_conns.heap_size--;

    log_info("HEAP SIZE: %d", bc->opt->hd_conns.heap_size);
    heap_bubble_down(&bc->opt->hd_conns, heap_idx);
    if (bc->opt->close_handler) bc->opt->close_handler(bc);
}


/* close connection, free memory */
void connection_close(basic_conn_t *bc) {
    passert(bc != NULL);
    ABORT_ON(bc->fd == ERROR, "FD ERROR");

    core_epoll_del(bc->opt->epoll_fd, bc->fd, 0, NULL);
    if (close(bc->fd) == ERROR) {
        log_info("The remote has closed, EXIT!");
        //raise(SIGINT); /* terminal, send signal */
    }

    connection_unregister(bc);
}

void server_connection_prune(net_ctx_t *opt) {
    while (opt->hd_conns.heap_size > 0 && opt->timeout) {
        basic_conn_t *bc = opt->hd_conns.hps[0]->obj;
        int64_t active_time = opt->hd_conns.hps[0]->factor;
        if (time(NULL) - active_time >= opt->timeout) {
            log_info("prune %p %d\n", bc, opt->hd_conns.heap_size);
            connection_close(bc);
        } else
            break;
    }
}
