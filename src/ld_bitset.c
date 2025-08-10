//
// Created by 邹嘉旭 on 2024/9/28.
//


#include "ld_bitset.h"

#include <ld_log.h>

ld_bitset_t *init_bitset(const size_t res_num, const size_t res_sz, init_resources_func init_func,
                         free_func free_func) {
    ld_bitset_t *bitset = malloc(sizeof(ld_bitset_t));
    bitset->res_num = res_num;
    bitset->res_sz = res_sz;
    bitset->bitset = calloc(BITSET_SIZE(res_num), sizeof(uint8_t));
    bitset->free_func = free_func;

    if (init_func && init_func(bitset) != LD_OK) {
        free(bitset->bitset);
        free(bitset);
        return NULL;
    }

    // if (init_func == NULL || free_func == NULL || init_func(bitset) != LD_OK) {
    //     free(bitset->bitset);
    //     free(bitset);
    //     return NULL;
    // }

    return bitset;
}

void free_bitset(void *v) {
    ld_bitset_t *bitset = v;
    if (bitset) {
        free(bitset->bitset);
        if (bitset->free_func) {
            bitset->free_func(bitset->resources);
        }
        free(bitset);
    }
}

l_err bs_record_by_index(ld_bitset_t *set, uint64_t i) {
    set->bitset[i / 8] |= (1 << (i % 8));
    return LD_OK;
}

l_err bs_alloc_resource(ld_bitset_t *set, void **res) {
    if (!set->resources) return LD_ERR_NULL;
    for (int i = 0; i < set->res_num; i++) {
        if ((set->bitset[i / 8] & (1 << (i % 8))) == 0) {
            set->bitset[i / 8] |= (1 << (i % 8));
            *res = RES_INDEX(set, i);

            return LD_OK;
        }
    }
    return LD_ERR_INVALID;
}

void bs_free_resource(ld_bitset_t *set, uint8_t index) {
    if (index < set->res_num) {
        set->bitset[index / 8] &= ~(1 << (index % 8));
    }
}

bool bs_judge_resource(ld_bitset_t *set, uint8_t index) {
    if ((set->bitset[index / 8] & (1 << (index % 8))) > 0) {
        return TRUE;
    }
    return FALSE;
}

bool bs_all_empty(ld_bitset_t *set) {
    for (int i = 0; i < set->res_num; i++) {
        if ((set->bitset[i / 8] & (1 << (i % 8))) == 1) {
            return FALSE; // 发现有未分配的资源
        }
    }
    // for (int i = 0; i < set->res_num; i++) {
    //     if ((set->bitset[i / 8] & (1 << (i % 8))) == 0) {
    //         return FALSE; // 发现有未分配的资源
    //     }
    // }
    return TRUE;
}

int bs_get_alloced(ld_bitset_t *set) {
    int ret = 0;
    // for (int i = 0; i < set->res_num; i++) {
    //     if ((set->bitset[i / 8] & (1 << (i % 8))) == 1) {
    //         ret++;
    //     }
    // }

    for (int i = 0; i < set->res_num; i++) {
        if ((set->bitset[i / 8] & (1 << (i % 8))) != 0) {
            ret++;
        }
    }
    return ret;
}

int bs_get_highest(ld_bitset_t *set) {
    int highest = 0;
    // for (int i = 0; i < set->res_num; i++) {
    //     if ((set->bitset[i / 8] & (1 << (i % 8))) == 1) {
    //         highest = i;
    //     }
    // }

    for (int i = 0; i < set->res_num; i++) {
        if ((set->bitset[i / 8] & (1 << (i % 8))) != 0) {
            highest = i;
        }
    }
    return highest;
}

int bs_get_lowest(ld_bitset_t *set) {
    // int lowest = -1;
    for (int i = 0; i < set->res_num; i++) {
        if ((set->bitset[i / 8] & (1 << (i % 8))) != 0) {
            // lowest = i;
            return i;
        }
    }
    return -1;
}

int bs_get_lowest_unalloced(ld_bitset_t *set) {
    // int lowest = -1;
    for (int i = 0; i < set->res_num; i++) {
        if ((set->bitset[i / 8] & (1 << (i % 8))) == 0) {
            // lowest = i;
            return i;
        }
    }
    return -1;
}

static void switch_endian(const uint8_t *src, size_t src_len, uint8_t *dst) {
    if (!src || !dst) return ;
    for (int i = 0; i < src_len; i++) {
        dst[i] = src[src_len - 1 - i];
    }
}

l_err bit_rightshift(uint8_t *src, size_t src_len, uint8_t *dst, size_t to_shift) {
    if (!src || !dst || src_len == 0) return LD_ERR_INTERNAL;
    if (to_shift == 0) {
        memcpy(dst, src, src_len);
        return LD_OK;
    }
    uint8_t *small_endian = calloc(src_len, sizeof(uint8_t));
    switch_endian(src, src_len, small_endian);

    for (int i = 0; i < to_shift; i++) {
        uint8_t to_add = 0;
        for (int j = 0; j < src_len; j++) {
            uint8_t last_bit = (small_endian[j] << 0x07) & 0x80;
            small_endian[j] = small_endian[j] >> 1;
            small_endian[j] += to_add;
            to_add  = last_bit;
        }
    }

    switch_endian(small_endian, src_len, dst);
    free(small_endian);
    return LD_OK;
}