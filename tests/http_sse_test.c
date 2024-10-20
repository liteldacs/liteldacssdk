//
// Created by 邹嘉旭 on 2024/4/30.
//
#include <stdio.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>

struct event_base *base;

// SSE事件发送函数
void send_sse_event(evutil_socket_t fd, short flags, void *args) {
    struct evhttp_request *req = args;
    struct evbuffer *buf = evbuffer_new();
    evbuffer_add_printf(buf, "event\n");
    evhttp_send_reply_chunk(req, buf);
    evbuffer_free(buf);
}

// HTTP请求处理函数
void http_request_handler(struct evhttp_request *req, void *arg) {
    fprintf(stderr, "!!!!!!\n");
    struct evkeyvalq *headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(headers, "Content-Type", "text/event-stream");
    evhttp_add_header(headers, "Cache-Control", "no-cache");
    evhttp_add_header(headers, "Connection", "keep-alive");
    evhttp_add_header(headers, "Access-Control-Allow-Origin", "*");

    // 发送初始化消息
    evhttp_send_reply_start(req, HTTP_OK, "");

    struct evbuffer *buf = evbuffer_new();
    evbuffer_add_printf(buf, "event\n");
    evhttp_send_reply_chunk(req, buf);
    evbuffer_free(buf);
    // 每秒发送一个事件
    // struct event *timer = event_new(base, -1, EV_PERSIST, send_sse_event, req);
    // struct timeval interval = {1, 0};
    // evtimer_add(timer, &interval);
}

int main() {
    base = event_base_new();
    struct evhttp *http_server = evhttp_new(base);

    // 设置HTTP请求处理函数
    evhttp_set_gencb(http_server, http_request_handler, NULL);

    // 启动HTTP服务器，监听指定的IP和端口
    evhttp_bind_socket(http_server, "0.0.0.0", 9456);

    // 进入libevent的事件循环，等待处理HTTP请求
    event_base_dispatch(base);

    // 清理资源
    evhttp_free(http_server);
    event_base_free(base);

    return 0;
}
