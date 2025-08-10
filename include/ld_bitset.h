//
// Created by 邹嘉旭 on 2024/9/28.
//

#ifndef LD_BITSET_H
#define LD_BITSET_H

#include "../global/ldacs_sim.h"

#define BITSET_SIZE(sz) ((sz) / 8 + ((sz) % 8 > 0))
#define RES_INDEX(res_name, i) (res_name)->resources + (i) * (res_name)->res_sz

typedef struct ld_bitset_s {
    size_t res_num;
    size_t res_sz;
    uint8_t *bitset;
    void *resources;
    free_func free_func;
} ld_bitset_t;

typedef l_err (*init_resources_func)(ld_bitset_t *);

ld_bitset_t *init_bitset(const size_t res_num, const size_t res_sz, init_resources_func init_func,
                         free_func free_func);

void free_bitset(void *v);

l_err bs_record_by_index(ld_bitset_t *set, uint64_t i);

l_err bs_alloc_resource(ld_bitset_t *set, void **res);

void bs_free_resource(ld_bitset_t *set, uint8_t index);

bool bs_judge_resource(ld_bitset_t *set, uint8_t index);

bool bs_all_empty(ld_bitset_t *set);

int bs_get_alloced(ld_bitset_t *set);

int bs_get_highest(ld_bitset_t *set);

int bs_get_lowest(ld_bitset_t *set);

int bs_get_lowest_unalloced(ld_bitset_t *set);

l_err bit_rightshift(uint8_t *src, size_t src_len, uint8_t *dst, size_t to_shift);


#endif //LD_BITSET_H
