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
            log_error("Bind Failed %d", local_port);
            close(fd);
            return -1;
        }
    }

    //TODO: 改成死循环，持续1min
    int i = RECONNECT;
    while (i--) {
        log_info("Trying to connect to remote `%s:%d` for %d time(s).", addr, remote_port, RECONNECT - i);
        if (connect(fd, (struct sockaddr *) to_conn_addr, sizeof(struct sockaddr_in)) >= 0) {
            log_info("Connected %d", fd);
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
            log_error("Bind Failed %d", local_port);
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
    if (opt->server_fd == ERROR) {
        log_error("Failed make server: %d", port);
        ABORT_ON(opt->server_fd == ERROR, "make_server");
    }
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

l_err defalut_send_pkt(basic_conn_t *bc, buffer_t *in_buf, l_err (*mid_func)(buffer_t *, void *),
                       void *args) {
    buffer_t *buf = init_buffer_unptr();
    if (mid_func) {
        mid_func(buf, args);
    }
    cat_to_buffer(buf, in_buf->ptr, in_buf->len);

    // 先添加到队列
    lfqueue_put(bc->write_pkts, buf);
    
    // 再检查队列状态
    if (!bc->current_write_buffer) {
        net_epoll_out(bc->opt->epoll_fd, bc);
    }
    // 如果已经在发送过程中，则不需要重复设置EPOLLOUT

    return LD_OK;
}

static void remove_from_buffer_front(buffer_t *buf, size_t count) {
    if (!buf || count == 0) return;

    if (count >= buf->len) {
        // 移除所有数据
        buf->len = 0;
        return;
    }

    // 移动剩余数据到开头
    memmove(buf->ptr, buf->ptr + count, buf->len - count);
    buf->len -= count;
}
// ============ 3. 处理完整数据包的函数 ============
static int process_complete_packets(basic_conn_t *bc) {
    if (!bc->recv_buffer) {
        return OK;
    }

    // 循环处理所有完整的数据包
    while (bc->recv_buffer->len > 0) {
        if (bc->reading_header) {
            // 正在读取包头（4字节长度）
            if (bc->recv_buffer->len < sizeof(uint32_t)) {
                // 包头不完整，等待更多数据
                break;
            }

            // 读取包长度
            uint32_t pkt_len_network;
            memcpy(&pkt_len_network, bc->recv_buffer->ptr, sizeof(uint32_t));
            bc->expected_pkt_len = ntohl(pkt_len_network);

            // 验证包长度
            if (bc->expected_pkt_len == 0) {
                log_warn("Received packet with zero length");
                // 移除包头，继续处理
                remove_from_buffer_front(bc->recv_buffer, sizeof(uint32_t));
                continue;
            }

            if (bc->expected_pkt_len > MAX_INPUT_BUFFER_SIZE) {
                log_error("Packet too large: %u bytes (max: %d)",
                         bc->expected_pkt_len, MAX_INPUT_BUFFER_SIZE);
                return ERROR;
            }

            // 移除包头
            remove_from_buffer_front(bc->recv_buffer, sizeof(uint32_t));
            bc->reading_header = false;

        } else {
            // 正在读取包体
            if (bc->recv_buffer->len < bc->expected_pkt_len) {
                // 包体不完整，等待更多数据
                break;
            }

            // 提取完整的包体
            if (bc->read_pkt) {
                free_buffer(bc->read_pkt);
            }
            bc->read_pkt = init_buffer_unptr();
            CLONE_TO_CHUNK(*bc->read_pkt, bc->recv_buffer->ptr, bc->expected_pkt_len);

            // 调用接收处理器
            if (bc->opt->recv_handler) {
                l_err result = bc->opt->recv_handler(bc);
                if (result == LD_ERR_INTERNAL) {
                    log_error("recv_handler failed for packet of %u bytes",
                             bc->expected_pkt_len);
                    free_buffer(bc->read_pkt);
                    bc->read_pkt = NULL;
                    return ERROR;
                }
            }

            // 清理已处理的包
            free_buffer(bc->read_pkt);
            bc->read_pkt = NULL;

            // 从缓冲区移除已处理的数据
            remove_from_buffer_front(bc->recv_buffer, bc->expected_pkt_len);

            // 准备读取下一个包头
            bc->reading_header = true;
            bc->expected_pkt_len = 0;
        }
    }

    return OK;
}
// ============ 2. 改进的 read_packet 函数 ============
static int read_packet(int fd, basic_conn_t *bc) {
    uint8_t temp[MAX_INPUT_BUFFER_SIZE];
    ssize_t len;
    bool has_read_any = false;

    // 初始化接收缓冲区（如果还没有的话）
    if (!bc->recv_buffer) {
        bc->recv_buffer = init_buffer_unptr();
        bc->reading_header = true;
        bc->expected_pkt_len = 0;
    }

    // 边缘触发模式：必须循环读取直到EAGAIN
    while (1) {
        len = read(fd, temp, sizeof(temp));

        if (len > 0) {
            has_read_any = true;

            // 将读取的数据追加到接收缓冲区
            cat_to_buffer(bc->recv_buffer, temp, len);

            // 尝试处理完整的数据包
            if (process_complete_packets(bc) == ERROR) {
                return ERROR;
            }

            // 继续读取更多数据
            continue;

        } else if (len == 0) {
            // 对端正常关闭连接
            log_info("Connection closed by peer, port: %d", get_port(bc));
            bc->state = CONN_STATE_CLOSED;
            return ERROR;

        } else { // len < 0
            // 检查错误类型
            int err = errno;

            if (err == EAGAIN || err == EWOULDBLOCK) {
                // 非阻塞socket暂时没有数据，这是正常的
                // 如果读取过数据，返回OK；否则也返回OK（表示没有新数据）
                // log_warn("AGAIN");

                return OK;

            } else if (err == EINTR) {
                // 被信号中断，继续尝试
                if (!has_read_any) {
                    // 如果还没读到任何数据，继续尝试
                    continue;
                }
                // 已经读到一些数据了，先处理这些
                return OK;

            } else if (err == ECONNRESET) {
                // 连接被对方重置
                log_warn("Connection reset by peer, port: %d", get_port(bc));
                bc->state = CONN_STATE_CLOSED;
                return ERROR;

            } else if (err == ETIMEDOUT) {
                // 连接超时
                log_warn("Connection timeout, port: %d", get_port(bc));
                return ERROR;

            } else if (err == EBADF || err == EINVAL) {
                // 无效的文件描述符，严重错误
                log_error("Invalid fd %d, error: %s", fd, strerror(err));
                return ERROR;

            } else {
                // 其他未预期的错误
                log_error("Unexpected read error on port %d: %s (errno=%d)",
                         get_port(bc), strerror(err), err);
                return ERROR;
            }
        }
    }
}



/**
 * Return:
 * OK: all data sent
 * AGAIN: haven't sent all data
 * ERROR: error
 */
static int write_packet(basic_conn_t *bc) {
    // 循环处理直到没有数据或遇到EAGAIN
    do {
        // 如果有未完成的发送缓冲区，先处理它
        if (bc->current_write_buffer) {
            size_t remaining = bc->current_write_buffer->len - bc->current_write_offset;
            ssize_t n = write(bc->fd,
                             (char*)bc->current_write_buffer->ptr + bc->current_write_offset,
                             remaining);

            if (n > 0) {
                bc->current_write_offset += n;
                if (bc->current_write_offset >= bc->current_write_buffer->len) {
                    // 当前缓冲区发送完成
                    free_buffer(bc->current_write_buffer);
                    bc->current_write_buffer = NULL;
                    bc->current_write_offset = 0;
                    // 继续处理队列中的下一个数据包
                    continue;
                } else {
                    // 还有数据未发送完
                    return AGAIN;
                }
            } else if (n == 0) {
                // 对于非阻塞socket，write返回0是异常情况
                log_error("Write returned 0, connection may be closed");
                return ERROR;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return AGAIN;
                } else if (errno == EINTR) {
                    // 被信号中断，继续循环重试
                    continue;
                } else if (errno == EPIPE || errno == ECONNRESET) {
                    // 连接已断开
                    log_error("Connection broken: %s", strerror(errno));
                    return ERROR;
                }
                log_error("Write error: %s", strerror(errno));
                return ERROR;
            }
        }
        
        // 处理队列中的新数据包
        if (lfqueue_size(bc->write_pkts) > 0) {
            buffer_t *b = lfqueue_deq(bc->write_pkts);

            if (!b) {
                log_error("Send buffer null: %d", lfqueue_size(bc->write_pkts));
                // 继续循环处理
                continue;
            }

            // 创建带长度头的完整包
            bc->current_write_buffer = init_buffer_unptr();
            uint32_t pkt_len = htonl(b->len);
            cat_to_buffer(bc->current_write_buffer, (uint8_t*)&pkt_len, sizeof(pkt_len));
            cat_to_buffer(bc->current_write_buffer, b->ptr, b->len);
            bc->current_write_offset = 0;

            free_buffer(b);

            // 尝试发送
            size_t remaining = bc->current_write_buffer->len;
            ssize_t n = write(bc->fd, (char*)bc->current_write_buffer->ptr, remaining);

            if (n > 0) {
                bc->current_write_offset += n;
                if (bc->current_write_offset >= bc->current_write_buffer->len) {
                    // 完整发送
                    free_buffer(bc->current_write_buffer);
                    bc->current_write_buffer = NULL;
                    bc->current_write_offset = 0;
                    // 继续处理下一个包
                    continue;
                } else {
                    // 部分发送，等待下次EPOLLOUT
                    return AGAIN;
                }
            } else if (n == 0) {
                return ERROR;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return AGAIN;
                } else if (errno == EINTR) {
                    // 被信号中断，继续循环
                    continue;
                }
                log_error("Write error: %s", strerror(errno));
                return ERROR;
            }
        } else {
            // 队列中没有更多数据
            break;
        }
    } while (1);

    return OK;
}

int request_handle(basic_conn_t *bc) {
    if (!bc || !bc->opt) {
        log_error("Invalid connection context");
        return ERROR;
    }

    // 检查连接状态
    if (bc->state != CONN_STATE_CONNECTED) {
        log_warn("Connection not in connected state");
        return ERROR;
    }

    // 检查fd有效性
    if (bc->fd < 0 || bc->fd == DEFAULT_FD) {
        log_error("Invalid fd: %d", bc->fd);
        return ERROR;
    }

    // 读取并处理数据
    if (bc->opt->recv_handler) {
        int result = read_packet(bc->fd, bc);
        return result;
    }

    return OK;
}



static int response_send_buffer(basic_conn_t *bc) {
    int status = write_packet(bc);

    switch (status) {
        case OK:
            // 检查是否还有待发送数据
            if (lfqueue_size(bc->write_pkts) == 0 && !bc->current_write_buffer) {
                // 所有数据发送完成
                bc->trans_done = TRUE;
                net_epoll_in(bc->opt->epoll_fd, bc);
            } else {
                // 还有数据要发送，保持EPOLLOUT状态
                bc->trans_done = FALSE;
                // 确保仍然监听EPOLLOUT事件
                net_epoll_out(bc->opt->epoll_fd, bc);
            }
            break;

        case AGAIN:
            // 发送缓冲区满，等待下次EPOLLOUT
            bc->trans_done = FALSE;
            // 保持当前EPOLLOUT状态，不修改
            break;

        case ERROR:
            bc->trans_done = TRUE;
            connecion_set_expired(bc);
            break;
    }

    return status;
}


int response_handle(basic_conn_t *bc) {
    int status = response_send_buffer(bc);

    // 根据状态决定后续行为
    switch (status) {
        case OK:
            if (bc->trans_done) {
                // 发送完成，重置连接状态
                if (bc->opt->reset_conn) {
                    bc->opt->reset_conn(bc);
                }
                net_epoll_in(bc->opt->epoll_fd, bc);
            }
            break;

        case AGAIN:
            // 继续等待EPOLLOUT事件，不修改epoll状态
            break;

        case ERROR:
            // 连接出错，标记为过期
            connecion_set_expired(bc);
            break;
    }

    return status;
}

void *net_setup(void *args) {
    int nfds;
    int i;
    net_ctx_t *net_ctx = args;

    while (TRUE) {
        nfds = core_epoll_wait(net_ctx->epoll_fd, net_ctx->epoll_events, MAX_EVENTS, -1);

        if (nfds == ERROR) {
            // if not caused by signal, cannot recover
            ERR_ON(errno != EINTR, "core_epoll_wait");
        }

        /* processing ready fd one by one */
        for (i = 0; i < nfds; i++) {
            struct epoll_event *curr_event = net_ctx->epoll_events + i;
            int fd = *((int *) curr_event->data.ptr);
            if (fd == net_ctx->server_fd) {
                // 循环处理accept 直到没有
                while (1) {
                    struct sockaddr_storage saddr;
                    socklen_t saddrlen = sizeof(struct sockaddr_storage);
                    int client_fd = accept(net_ctx->server_fd, (struct sockaddr *)&saddr, &saddrlen);

                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;  // 没有更多连接
                        }
                        if (errno != EINTR) {
                            log_error("accept error: %s", strerror(errno));
                            break;
                        }
                        continue;  // EINTR, 重试
                    }

                    // 处理新连接
                    net_ctx->accept_handler(net_ctx, client_fd, &saddr);
                }
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

                if (has_error) {
                    connecion_set_expired(bc);
                    net_epoll_in(bc->opt->epoll_fd, bc); // 确保恢复到IN状态
                } else if (should_reactivate) {
                    connecion_set_reactivated(bc);
                }
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
    struct epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET;  // 只监听写事件和边缘触发
    ev.data.ptr = bc;
    if (epoll_ctl(e_fd, EPOLL_CTL_MOD, bc->fd, &ev) == 0) {
        bc->event.events = ev.events;
    } else {
        log_error("Failed to modify epoll: %s", strerror(errno));
    }
}

void net_epoll_in(int e_fd, basic_conn_t *bc) {
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = bc;
    if (epoll_ctl(e_fd, EPOLL_CTL_MOD, bc->fd, &ev) == 0) {
        bc->event.events = ev.events;
    } else {
        perror("epoll_ctl(IN)");
    }
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
        start[i] = (uint8_t) (addr_int >> (BITS_PER_BYTE * i));
    }
}

bool init_basic_conn_server(basic_conn_t *bc, net_ctx_t *ctx, sock_roles socket_role, int fd, struct sockaddr_storage *saddr) {
    bc->rp = get_role_propt(socket_role);
    bc->fd = fd;
    if (bc->fd == ERROR || bc->fd < 0) {
        log_error("Failed to create connection fd");
        return FALSE;
    }
    bc->opt = ctx;

    memcpy(&bc->saddr, saddr, sizeof(struct sockaddr_storage));

    return init_basic_conn(bc);
}

bool init_basic_conn_client(basic_conn_t *bc, net_ctx_t *ctx, sock_roles socket_role ) {
    bc->rp = get_role_propt(socket_role);
    bc->fd = bc->rp->handler(bc);
    bc->opt = ctx;
    return init_basic_conn(bc);
}

bool init_basic_conn(basic_conn_t *bc) {
    do {
        // 清零整个结构体

        // bc->fd = DEFAULT_FD;
        // bc->rp = get_role_propt(socket_role);
        // bc->fd = bc->rp->handler(bc);

        ABORT_ON(bc->opt->epoll_fd == 0 || bc->opt->epoll_fd == ERROR, "illegal epoll fd");

        if (connection_register(bc, time(NULL)) == ERROR) {
            log_error("Failed to register connection");
            break;
        }

        // 设置非阻塞
        if (set_fd_nonblocking(bc->fd) == ERROR) {
            log_error("Failed to set non-blocking mode");
            break;
        }

        // 设置TCP_NODELAY
        connection_set_nodelay(bc->fd);

        // 初始化队列和缓冲区
        bc->write_pkts = lfqueue_init();
        if (!bc->write_pkts) {
            log_error("Failed to create write queue");
            break;
        }

        // 初始化接收相关字段
        bc->recv_buffer = NULL;  // 延迟初始化，在第一次读取时创建
        bc->read_pkt = NULL;
        bc->reading_header = true;
        bc->expected_pkt_len = 0;

        // 初始化发送相关字段
        bc->current_write_buffer = NULL;
        bc->current_write_offset = 0;
        bc->trans_done = false;

        // 设置连接状态
        bc->state = CONN_STATE_CONNECTED;

        // 添加到epoll
        if (net_epoll_add(bc->opt->epoll_fd, bc, EPOLLIN | EPOLLET, &bc->event) == ERROR) {
            log_error("Failed to add to epoll");
            break;
        }

        return TRUE;

    } while (0);

    // 清理资源
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


void connection_close(basic_conn_t *bc) {
    if (!bc) return;

    // 标记状态
    bc->state = CONN_STATE_CLOSING;

    // 从epoll移除并关闭socket
    if (bc->fd != DEFAULT_FD && bc->fd != ERROR && bc->fd >= 0) {
        core_epoll_del(bc->opt->epoll_fd, bc->fd, 0, NULL);
        close(bc->fd);
        bc->fd = DEFAULT_FD;
    }

    // 清理接收缓冲区
    if (bc->recv_buffer) {
        free_buffer(bc->recv_buffer);
        bc->recv_buffer = NULL;
    }

    // 清理当前读取的包
    if (bc->read_pkt) {
        free_buffer(bc->read_pkt);
        bc->read_pkt = NULL;
    }

    // 清理当前发送缓冲区
    if (bc->current_write_buffer) {
        free_buffer(bc->current_write_buffer);
        bc->current_write_buffer = NULL;
    }

    // 清理发送队列
    if (bc->write_pkts) {
        buffer_t *buf;
        while ((buf = lfqueue_deq(bc->write_pkts)) != NULL) {
            free_buffer(buf);
        }
        lfqueue_destroy(bc->write_pkts);
        bc->write_pkts = NULL;
    }

    // 重置状态
    bc->reading_header = true;
    bc->expected_pkt_len = 0;
    bc->current_write_offset = 0;
    bc->trans_done = false;
    bc->state = CONN_STATE_CLOSED;

    // 从连接管理器注销
    connection_unregister(bc);
}
void server_connection_prune(net_ctx_t *opt) {
    time_t current_time = time(NULL);
    int pruned_count = 0;
    // 限制每次调用最多清理的连接数，避免长时间占用CPU
    while (opt->hd_conns.heap_size > 0 && opt->timeout && pruned_count < 100) {
        basic_conn_t *bc = opt->hd_conns.hps[0]->obj;
        int64_t active_time = opt->hd_conns.hps[0]->factor;
        if (current_time - active_time >= opt->timeout) {
            log_info("prune %p %d\n", bc, opt->hd_conns.heap_size);
            connection_close(bc);
            pruned_count++;
        } else {
            break;
        }
    }
}


uint16_t get_port(basic_conn_t *bc) {
    return ntohs(((struct sockaddr_in *) &bc->saddr)->sin_port);
}
