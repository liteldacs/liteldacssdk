//
// Created by 邹嘉旭 on 2024/10/17.
//

#ifndef LD_UTILS_H
#define LD_UTILS_H

#include "../global/ldacs_sim.h"
#include "ld_buffer.h"
#include "ld_util_def.h"

void get_time(char *time_str, enum TIME_MOD t_mod);

void generate_rand(uint8_t *rand, size_t len);

uint64_t generate_urand(size_t rand_bits_sz);

void generate_nrand(uint8_t *rand, size_t sz);

int32_t memrchr(const buffer_t *buf, uint8_t target);

int bytes_len(size_t n);

#endif //LD_UTILS_H
