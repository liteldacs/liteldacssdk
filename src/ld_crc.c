//
// Created by 邹嘉旭 on 2024/3/19.
//

#include "ld_crc.h"

/**
 * Calculate CRC-8 in 0x97 polynomial
 * @param size
 * @param buf
 */
uint8_t cal_crc_8bits(uint8_t *start, const uint8_t *cur){
    uint8_t crc = 0x00;
    uint8_t *iter = start;
    size_t len = cur - start;

    while(len--){
        crc = crc ^ *iter++; // Apply Byte
        crc = crc8_table[crc & 0xFF]; // One round of 8-bits
    }

    return crc;
}
/**
 * Calculate CRC-32 in 0xFA567D89 polynomial
 * @param size
 * @param buf
 */
uint32_t cal_crc_32bits(uint8_t *start, const uint8_t *cur){
    uint32_t crc = 0x00000000;
    uint8_t *iter = start;
    size_t len = cur - start;

    while(len--){
        crc = (crc >> 8) ^ crc32_table[(crc ^ (uint32_t) *iter++) & 0x000000FFul ];
    }

    return (crc ^ 0xFFFFFFFFul);
}