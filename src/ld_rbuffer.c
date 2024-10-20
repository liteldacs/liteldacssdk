//
// Created by 邹嘉旭 on 2024/10/8.
//
#include "ld_rbuffer.h"

ld_rbuffer *ld_rbuffer_init(int size) {
    ld_rbuffer *rb = (ld_rbuffer *) malloc(sizeof(ld_rbuffer));
    rb->head = NULL;
    rb->tail = NULL;
    rb->count = 0;
    rb->size = size;
    return rb;
}

// 向缓冲区前面插入元素 (push_front)
l_err ld_rbuffer_push_front(ld_rbuffer *rb, void *data) {
    if (rb->count >= rb->size) {
        return LD_ERR_INTERNAL;
    }

    rnode_t *new_node = (rnode_t *) malloc(sizeof(rnode_t));
    new_node->data = data;

    if (rb->count == 0) {
        // 如果是第一个元素
        rb->head = new_node;
        rb->tail = new_node;
        new_node->next = new_node;
        new_node->prev = new_node;
    } else {
        new_node->next = rb->head;
        new_node->prev = rb->tail;
        rb->tail->next = new_node;
        rb->head->prev = new_node;
        rb->head = new_node;
    }
    rb->count++;
    return LD_OK;
}

// 向缓冲区尾部插入元素 (push_back)
l_err ld_rbuffer_push_back(ld_rbuffer *rb, void *data) {
    if (rb->count >= rb->size) {
        return LD_ERR_INTERNAL;
    }

    rnode_t *new_node = (rnode_t *) malloc(sizeof(rnode_t));
    new_node->data = data;

    if (rb->count == 0) {
        // 如果是第一个元素
        rb->head = new_node;
        rb->tail = new_node;
        new_node->next = new_node;
        new_node->prev = new_node;
    } else {
        new_node->next = rb->head;
        new_node->prev = rb->tail;
        rb->tail->next = new_node;
        rb->head->prev = new_node;
        rb->tail = new_node;
    }
    rb->count++;
    return LD_OK;
}

// 删除并返回缓冲区的头部元素 (pop)
l_err ld_rbuffer_pop(ld_rbuffer *rb, void **data) {
    if (rb->count == 0) {
        *data = NULL;
        return LD_ERR_INTERNAL;
    }

    rnode_t *pop_node = rb->head;
    *data = pop_node->data;

    if (rb->count == 1) {
        // 如果只有一个元素
        rb->head = NULL;
        rb->tail = NULL;
    } else {
        rb->head = pop_node->next;
        rb->tail->next = rb->head;
        rb->head->prev = rb->tail;
    }

    free(pop_node);
    rb->count--;
    return LD_OK;
}

// 获取缓冲区的头部元素 (get_front)
l_err ld_rbuffer_get_front(ld_rbuffer *rb, void **data) {
    if (rb->count == 0) {
        *data = NULL;
        return LD_ERR_INTERNAL;
    }
    *data = rb->head->data;
    return LD_OK;
}

// 释放整个缓冲区
void ld_rbuffer_free(ld_rbuffer *rb) {
    while (rb->count > 0) {
        void *tmp;
        ld_rbuffer_pop(rb, &tmp); // 逐个删除元素
    }
    free(rb);
}

void ring_buffer_traverse(ld_rbuffer *rb, void (*callback)(void *)) {
    if (rb->count == 0) {
        log_warn("Buffer is empty!\n");
        return;
    }

    rnode_t *current = rb->head;
    int count = 0;

    // 遍历直到回到起始节点
    while (count < rb->count) {
        callback(current->data); // 调用回调函数处理当前节点的数据
        current = current->next; // 移动到下一个节点
        count++;
    }
}

bool ld_rbuffer_check_exist(ld_rbuffer *rb, void *ptr, cmp_factor factor) {
    rnode_t *current = rb->head;
    int count = 0;

    // 遍历直到回到起始节点
    while (count < rb->count) {
        if (factor(current->data, ptr) == TRUE) return TRUE;
        current = current->next; // 移动到下一个节点
        count++;
    }
    return FALSE;
}

size_t ld_rbuffer_count(ld_rbuffer *rb) {
    return rb->count;
}
