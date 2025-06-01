//
// Created by 邹嘉旭 on 2024/9/2.
//
#include <ld_bitset.h>
#include <ld_log.h>
#include <stdio.h>
#include <stdint.h>

// #define NUM_RESOURCES 32
// #define BITSET_SIZE (NUM_RESOURCES / 8 + (NUM_RESOURCES % 8 > 0))
//
// uint8_t bitset[BITSET_SIZE] = {0}; // 位图数组
// int resources[NUM_RESOURCES] = {
//     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
//     16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
// };
//
// int allocateResource() {
//     for (int i = 0; i < NUM_RESOURCES; i++) {
//         if ((bitset[i / 8] & (1 << (i % 8))) == 0) {
//             bitset[i / 8] |= (1 << (i % 8));
//             printf("资源 %d 已分配。\n", resources[i]);
//             return i;
//         }
//     }
//     printf("没有可用资源。\n");
//     return -1;
// }
//
// void freeResource(int index) {
//     if (index >= 0 && index < NUM_RESOURCES) {
//         bitset[index / 8] &= ~(1 << (index % 8));
//         printf("资源 %d 已释放。\n", resources[index]);
//     } else {
//         printf("无效的资源索引。\n");
//     }
// }
//
// void printResources() {
//     printf("资源状态:\n");
//     for (int i = 0; i < NUM_RESOURCES; i++) {
//         printf("资源 %d: %s\n", resources[i], (bitset[i / 8] & (1 << (i % 8))) ? "已分配" : "未分配");
//     }
//     printf("\n");
// }
//
// void judgeResources(int index) {
//     if ((bitset[index / 8] & (1 << (index % 8))) > 0) {
//         printf("YES\n");
//         return;
//     }
//     printf("NO\n");
// }

int main() {
    // allocateResource(); // 分配一个资源
    // allocateResource(); // 分配另一个资源
    // allocateResource(); // 分配另一个资源
    // allocateResource(); // 分配另一个资源
    // printResources(); // 打印资源状态
    // freeResource(1); // 释放一个资源
    // freeResource(2); // 释放一个资源
    // allocateResource(); // 分配另一个资源
    // printResources(); // 再次打印资源状态
    // judgeResources(1);
    // judgeResources(2);
    // judgeResources(3);
    // judgeResources(18);
    // judgeResources(0);
    //
    uint8_t src[4] = {0x02, 0x04, 0x4E, 0x0F};
    uint8_t dst[4] = {0};
    bit_rightshift(src, 4, dst, 1);
    log_buf(LOG_INFO, "AAAA", dst, 4);
    return 0;
}

