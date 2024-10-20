//
// Created by 邹嘉旭 on 2024/10/8.
//

#ifndef LD_BUF_H
#define LD_BUF_H

#include "../global/ldacs_sim.h"
#include "ld_log.h"

#define QUEUE_SIZE 4096

typedef bool (*cmp_factor)(const void *a, const void *b);

// 定义节点结构
typedef struct rnode_s {
    void *data; // 指向数据的指针
    struct rnode_s *prev; // 前一个节点
    struct rnode_s *next; // 后一个节点
} rnode_t;

// 定义环形缓冲区结构
typedef struct {
    rnode_t *head; // 指向链表的头节点
    rnode_t *tail; // 指向链表的尾节点
    int count; // 当前节点数
    int size; // 缓冲区的最大容量
} ld_rbuffer;

ld_rbuffer *ld_rbuffer_init(int size);

l_err ld_rbuffer_push_front(ld_rbuffer *rb, void *data);

l_err ld_rbuffer_push_back(ld_rbuffer *rb, void *data);

l_err ld_rbuffer_pop(ld_rbuffer *rb, void **data);

l_err ld_rbuffer_get_front(ld_rbuffer *rb, void **data);

void ld_rbuffer_free(ld_rbuffer *rb);

bool ld_rbuffer_check_exist(ld_rbuffer *rb, void *ptr, cmp_factor factor);

size_t ld_rbuffer_count(ld_rbuffer *rb);

#endif //LD_BUF_H
