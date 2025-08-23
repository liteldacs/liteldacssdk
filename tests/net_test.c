//
// ld_net 压力测试和准确性验证程序
// Created by jiaxv on 2025-08-22
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>

#include "ld_net.h"
#include "ld_log.h"
#include "ld_thread.h"
#include "ld_buffer.h"
#include "ld_epoll.h"
#include "ld_heap.h"

// 测试配置
#define TEST_PORT 8888
#define TEST_HOST "127.0.0.1"
#define MAX_CLIENTS 100
#define TEST_DURATION_SEC 10
#define PACKET_SIZE_MIN 64
#define PACKET_SIZE_MAX 8192
#define PACKETS_PER_CLIENT 1000

// 统计信息
typedef struct {
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t errors;
    uint64_t checksum_errors;
    double start_time;
    double end_time;
    pthread_mutex_t mutex;
} test_stats_t;

// 测试数据包结构
typedef struct {
    uint32_t seq_num;
    uint32_t client_id;
    uint32_t data_size;
    uint32_t checksum;
    uint8_t data[];
} test_packet_t;

// 客户端上下文
typedef struct {
    int client_id;
    uint32_t next_seq;
    uint32_t expected_seq;
    basic_conn_t *conn;
    net_ctx_t *net_ctx;
    test_stats_t *stats;
    bool running;
} client_ctx_t;

// 服务器上下文
typedef struct {
    net_ctx_t *net_ctx;
    test_stats_t *stats;
    bool running;
} server_ctx_t;

static test_stats_t g_stats = {0};
static volatile bool g_test_running = true;
static pthread_t g_server_thread;
static pthread_t g_client_threads[MAX_CLIENTS];
static client_ctx_t g_client_contexts[MAX_CLIENTS];
static server_ctx_t g_server_context;

// 工具函数
static double get_time_inner() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

static uint32_t calculate_checksum(const uint8_t *data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

static void update_stats(test_stats_t *stats, bool is_send, size_t bytes, bool error, bool checksum_error) {
    pthread_mutex_lock(&stats->mutex);
    if (is_send) {
        stats->packets_sent++;
        stats->bytes_sent += bytes;
    } else {
        stats->packets_received++;
        stats->bytes_received += bytes;
    }
    if (error) stats->errors++;
    if (checksum_error) stats->checksum_errors++;
    pthread_mutex_unlock(&stats->mutex);
}

// 创建测试数据包
static buffer_t* create_test_packet(uint32_t seq_num, uint32_t client_id, size_t data_size) {
    size_t total_size = sizeof(test_packet_t) + data_size;
    buffer_t *buf = init_buffer_unptr();

    test_packet_t *pkt = malloc(total_size);
    pkt->seq_num = htonl(seq_num);
    pkt->client_id = htonl(client_id);
    pkt->data_size = htonl(data_size);

    // 填充测试数据
    for (size_t i = 0; i < data_size; i++) {
        pkt->data[i] = (uint8_t)(seq_num + client_id + i);
    }

    pkt->checksum = htonl(calculate_checksum(pkt->data, data_size));

    cat_to_buffer(buf, (uint8_t*)pkt, total_size);
    free(pkt);

    return buf;
}

// 验证接收到的数据包
static bool verify_packet(const uint8_t *data, size_t len, uint32_t *seq_num, uint32_t *client_id) {
    if (len < sizeof(test_packet_t)) {
        log_error("Packet too small: %zu bytes", len);
        return false;
    }


    test_packet_t *pkt = (test_packet_t*)data;
    *seq_num = ntohl(pkt->seq_num);
    *client_id = ntohl(pkt->client_id);
    uint32_t data_size = ntohl(pkt->data_size);
    uint32_t received_checksum = ntohl(pkt->checksum);

    if (sizeof(test_packet_t) + data_size != len) {
        log_error("Invalid packet size: expected %zu, got %zu",
                 sizeof(test_packet_t) + data_size, len);
        return false;
    }

    uint32_t calculated_checksum = calculate_checksum(pkt->data, data_size);
    if (calculated_checksum != received_checksum) {
        log_error("Checksum mismatch: expected %u, got %u",
                 calculated_checksum, received_checksum);
        return false;
    }

    return true;
}

// 服务器接收处理器
static l_err server_recv_handler(basic_conn_t *bc) {
    if (!bc->read_pkt || !bc->read_pkt->ptr) {
        return LD_ERR_INTERNAL;
    }

    uint32_t seq_num, client_id;
    bool valid = verify_packet(bc->read_pkt->ptr, bc->read_pkt->len, &seq_num, &client_id);


    // log_warn("========================= %d %d", seq_num, client_id);
    // update_stats(&g_stats, false, bc->read_pkt->len, !valid, !valid);

    if (valid) {
        // 回显数据包
        buffer_t *echo_buf = init_buffer_unptr();
        cat_to_buffer(echo_buf, bc->read_pkt->ptr, bc->read_pkt->len);

        if (bc->opt->send_handler) {
            bc->opt->send_handler(bc, echo_buf, NULL, NULL);
        }

        free_buffer(echo_buf);
    }

    return LD_OK;
}

// 客户端接收处理器
static l_err client_recv_handler(basic_conn_t *bc) {
    client_ctx_t *ctx = (client_ctx_t*)bc->opt->arg;

    if (!bc->read_pkt || !bc->read_pkt->ptr) {
        return LD_ERR_INTERNAL;
    }

    uint32_t seq_num, client_id;
    bool valid = verify_packet(bc->read_pkt->ptr, bc->read_pkt->len, &seq_num, &client_id);

    // fprintf(stderr, "++++++++++++%d %d\n", seq_num, client_id);

    if (valid && client_id == ctx->client_id) {
        if (seq_num == ctx->expected_seq) {
            ctx->expected_seq++;

            update_stats(ctx->stats, false, bc->read_pkt->len, false, false);
        } else {
            log_warn("Client %d: sequence mismatch, expected %u, got %u",
                    ctx->client_id, ctx->expected_seq, seq_num);
            update_stats(ctx->stats, false, bc->read_pkt->len, true, false);
        }
    } else {
        update_stats(ctx->stats, false, bc->read_pkt->len, true, !valid);
    }

    return LD_OK;
}

// 服务器连接处理器
static void* server_conn_handler(net_ctx_t *ctx, char *remote_addr, int remote_port, int local_port) {
    basic_conn_t *bc = malloc(sizeof(basic_conn_t));
    memset(bc, 0, sizeof(basic_conn_t));

    bc->remote_addr = strdup(remote_addr);
    bc->remote_port = remote_port;
    bc->local_port = local_port;

    if (!init_basic_conn_client(bc, ctx, LD_TCP_CLIENT)) {
        free(bc->remote_addr);
        free(bc);
        return NULL;
    }

    return bc;
}

// 服务器接受处理器
static l_err server_accept_handler(net_ctx_t *ctx, int fd, struct sockaddr_storage *saddr) {
    basic_conn_t *bc = malloc(sizeof(basic_conn_t));
    memset(bc, 0, sizeof(basic_conn_t));

    if (!init_basic_conn_server(bc, ctx, LD_TCP_SERVER, fd,saddr )) {
        free(bc);
        return LD_ERR_INTERNAL;
    }

    log_info("New client connected from port %d", get_port(bc));
    return LD_OK;
}

// 客户端连接处理器
static void* client_conn_handler(net_ctx_t *ctx, char *remote_addr, int remote_port, int local_port) {
    basic_conn_t *bc = malloc(sizeof(basic_conn_t));
    memset(bc, 0, sizeof(basic_conn_t));

    bc->remote_addr = strdup(remote_addr);
    bc->remote_port = remote_port;
    bc->local_port = local_port;

    if (!init_basic_conn_client(bc, ctx, LD_TCP_CLIENT)) {
        free(bc->remote_addr);
        free(bc);
        return NULL;
    }

    return bc;
}

// 服务器线程函数
static void* server_thread_func(void *arg) {
    server_ctx_t *server_ctx = (server_ctx_t*)arg;

    // 初始化网络上下文
    server_ctx->net_ctx = malloc(sizeof(net_ctx_t));
    memset(server_ctx->net_ctx, 0, sizeof(net_ctx_t));

    strcpy(server_ctx->net_ctx->name, "test_server");
    server_ctx->net_ctx->epoll_fd = core_epoll_create(0, -1);
    server_ctx->net_ctx->timeout = 300; // 5分钟超时
    server_ctx->net_ctx->arg = server_ctx;

    // 初始化连接堆
    init_heap_desc(&server_ctx->net_ctx->hd_conns);

    // 设置处理器
    server_ctx->net_ctx->recv_handler = server_recv_handler;
    server_ctx->net_ctx->send_handler = defalut_send_pkt;
    server_ctx->net_ctx->conn_handler = server_conn_handler;
    server_ctx->net_ctx->accept_handler = server_accept_handler;

    // 启动服务器
    server_entity_setup(TEST_PORT, server_ctx->net_ctx, LD_TCP_SERVER);

    log_info("Server started on port %d", TEST_PORT);

    // 运行事件循环
    net_setup(server_ctx->net_ctx);

    return NULL;
}

// 客户端线程函数
static void* client_thread_func(void *arg) {
    client_ctx_t *client_ctx = (client_ctx_t*)arg;

    // 等待服务器启动
    sleep(1);

    // 初始化网络上下文
    client_ctx->net_ctx = malloc(sizeof(net_ctx_t));
    memset(client_ctx->net_ctx, 0, sizeof(net_ctx_t));

    snprintf(client_ctx->net_ctx->name, sizeof(client_ctx->net_ctx->name),
             "client_%d", client_ctx->client_id);
    client_ctx->net_ctx->epoll_fd = core_epoll_create(0, -1);
    client_ctx->net_ctx->timeout = 300;
    client_ctx->net_ctx->arg = client_ctx;

    // 初始化连接堆
    init_heap_desc(&client_ctx->net_ctx->hd_conns);

    // 设置处理器
    client_ctx->net_ctx->recv_handler = client_recv_handler;
    client_ctx->net_ctx->send_handler = defalut_send_pkt;
    client_ctx->net_ctx->conn_handler = client_conn_handler;

    // 连接到服务器
    client_ctx->conn = (basic_conn_t*)client_entity_setup(
        client_ctx->net_ctx, TEST_HOST, TEST_PORT, 0);

    if (!client_ctx->conn) {
        log_error("Client %d failed to connect", client_ctx->client_id);
        return NULL;
    }

    log_info("Client %d connected", client_ctx->client_id);

    // 发送测试数据包
    for (int i = 0; i < PACKETS_PER_CLIENT && g_test_running; i++) {
        // size_t data_size = PACKET_SIZE_MIN +
        //     (rand() % (PACKET_SIZE_MAX - PACKET_SIZE_MIN + 1));

        size_t data_size = 2500;


        buffer_t *pkt = create_test_packet(client_ctx->next_seq,
                                         client_ctx->client_id, data_size);

        if (client_ctx->net_ctx->send_handler) {
            l_err result = client_ctx->net_ctx->send_handler(
                client_ctx->conn, pkt, NULL, NULL);

            if (result == LD_OK) {
                update_stats(client_ctx->stats, true, pkt->len, false, false);
                client_ctx->next_seq++;
            } else {
                update_stats(client_ctx->stats, true, pkt->len, true, false);
            }
        }

        free_buffer(pkt);

        // 控制发送速率
        usleep(1000); // 1ms间隔
    }

    // 运行事件循环处理接收
    pthread_t net_thread;
    pthread_create(&net_thread, NULL, (void*(*)(void*))net_setup, client_ctx->net_ctx);

    // 等待测试结束
    while (g_test_running) {
        sleep(1);
    }

    pthread_cancel(net_thread);
    pthread_join(net_thread, NULL);

    return NULL;
}
// 信号处理函数
static void signal_handler(int sig) {
    log_info("Received signal %d, stopping test...", sig);
    g_test_running = false;
}

// 打印统计信息
static void print_stats(test_stats_t *stats) {
    pthread_mutex_lock(&stats->mutex);

    double duration = stats->end_time - stats->start_time;
    double send_rate = duration > 0 ? stats->packets_sent / duration : 0;
    double recv_rate = duration > 0 ? stats->packets_received / duration : 0;
    double send_throughput = duration > 0 ? (stats->bytes_sent * 8.0) / (duration * 1024 * 1024) : 0;
    double recv_throughput = duration > 0 ? (stats->bytes_received * 8.0) / (duration * 1024 * 1024) : 0;
    double packet_loss = stats->packets_sent > 0 ?
        (double)(stats->packets_sent - stats->packets_received) / stats->packets_sent * 100 : 0;

    printf("\n=== LD_NET 压力测试结果 ===\n");
    printf("测试时长: %.2f 秒\n", duration);
    printf("发送数据包: %lu 个\n", stats->packets_sent);
    printf("接收数据包: %lu 个\n", stats->packets_received);
    printf("发送字节数: %lu 字节 (%.2f MB)\n", stats->bytes_sent, stats->bytes_sent / (1024.0 * 1024.0));
    printf("接收字节数: %lu 字节 (%.2f MB)\n", stats->bytes_received, stats->bytes_received / (1024.0 * 1024.0));
    printf("发送速率: %.2f 包/秒\n", send_rate);
    printf("接收速率: %.2f 包/秒\n", recv_rate);
    printf("发送吞吐量: %.2f Mbps\n", send_throughput);
    printf("接收吞吐量: %.2f Mbps\n", recv_throughput);
    printf("丢包率: %.2f%%\n", packet_loss);
    printf("错误数: %lu\n", stats->errors);
    printf("校验和错误: %lu\n", stats->checksum_errors);

    // 准确性评估
    if (stats->errors == 0 && stats->checksum_errors == 0) {
        printf("✓ 数据传输准确性: 完美\n");
    } else if (stats->errors + stats->checksum_errors < stats->packets_received * 0.01) {
        printf("⚠ 数据传输准确性: 良好 (错误率 < 1%%)\n");
    } else {
        printf("✗ 数据传输准确性: 较差 (错误率 >= 1%%)\n");
    }

    // 性能评估
    if (send_rate > 1000 && recv_rate > 1000) {
        printf("✓ 性能表现: 优秀 (>1000 包/秒)\n");
    } else if (send_rate > 100 && recv_rate > 100) {
        printf("⚠ 性能表现: 良好 (>100 包/秒)\n");
    } else {
        printf("✗ 性能表现: 需要优化 (<100 包/秒)\n");
    }

    printf("========================\n\n");

    pthread_mutex_unlock(&stats->mutex);
}

// 监控线程函数
static void* monitor_thread_func(void *arg) {
    test_stats_t *stats = (test_stats_t*)arg;

    while (g_test_running) {
        sleep(2);

        pthread_mutex_lock(&stats->mutex);
        printf("[监控] 发送: %lu, 接收: %lu, 错误: %lu\n",
               stats->packets_sent, stats->packets_received, stats->errors);
        pthread_mutex_unlock(&stats->mutex);
    }

    return NULL;
}

// 压力测试主函数
static int run_stress_test(int num_clients) {
    printf("开始 LD_NET 压力测试...\n");
    printf("配置: %d 客户端, 每客户端 %d 包, 包大小 %d-%d 字节\n",
           num_clients, PACKETS_PER_CLIENT, PACKET_SIZE_MIN, PACKET_SIZE_MAX);

    // 初始化统计信息
    memset(&g_stats, 0, sizeof(g_stats));
    pthread_mutex_init(&g_stats.mutex, NULL);
    g_stats.start_time = get_time_inner();

    // 初始化服务器上下文
    memset(&g_server_context, 0, sizeof(g_server_context));
    g_server_context.stats = &g_stats;
    g_server_context.running = true;

    // 启动服务器线程
    if (pthread_create(&g_server_thread, NULL, server_thread_func, &g_server_context) != 0) {
        log_error("Failed to create server thread");
        return -1;
    }

    // 等待服务器启动
    sleep(2);

    // 启动客户端线程
    for (int i = 0; i < num_clients; i++) {
        memset(&g_client_contexts[i], 0, sizeof(client_ctx_t));
        g_client_contexts[i].client_id = i;
        g_client_contexts[i].next_seq = 0;
        g_client_contexts[i].expected_seq = 0;
        g_client_contexts[i].stats = &g_stats;
        g_client_contexts[i].running = true;

        if (pthread_create(&g_client_threads[i], NULL, client_thread_func, &g_client_contexts[i]) != 0) {
            log_error("Failed to create client thread %d", i);
            continue;
        }
    }

    // 启动监控线程
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor_thread_func, &g_stats);

    // 运行指定时间
    sleep(TEST_DURATION_SEC);

    // 停止测试
    g_test_running = false;
    g_stats.end_time = get_time_inner();

    // 等待所有线程结束
    for (int i = 0; i < num_clients; i++) {
        pthread_join(g_client_threads[i], NULL);
    }

    pthread_cancel(g_server_thread);
    pthread_join(g_server_thread, NULL);

    pthread_cancel(monitor_thread);
    pthread_join(monitor_thread, NULL);

    // 打印结果
    print_stats(&g_stats);

    pthread_mutex_destroy(&g_stats.mutex);

    return 0;
}

// 主函数
int main(int argc, char *argv[]) {
    int num_clients = 100;

    // 解析命令行参数
    if (argc > 1) {
        num_clients = atoi(argv[1]);
        if (num_clients <= 0 || num_clients > MAX_CLIENTS) {
            printf("客户端数量必须在 1-%d 之间\n", MAX_CLIENTS);
            return -1;
        }
    }

    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 初始化随机种子
    srand(time(NULL));

    // 初始化日志系统
    log_set_level(LOG_INFO);

    printf("LD_NET 压力测试和准确性验证程序\n");
    printf("使用方法: %s [客户端数量]\n", argv[0]);
    printf("默认客户端数量: %d\n", num_clients);
    printf("按 Ctrl+C 提前停止测试\n\n");

    // 运行压力测试
    int result = run_stress_test(num_clients);

    if (result == 0) {
        printf("测试完成!\n");
    } else {
        printf("测试失败!\n");
    }

    return result;
}