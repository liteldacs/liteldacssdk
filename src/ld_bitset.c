//
// Created by 邹嘉旭 on 2024/9/28.
//


#include "ld_bitset.h"


ld_bitset_t *init_bitset(const size_t res_num, const size_t res_sz, init_resources_func init_func,
                         free_func free_func) {
    ld_bitset_t *bitset = malloc(sizeof(ld_bitset_t));
    bitset->res_num = res_num;
    bitset->res_sz = res_sz;
    bitset->bitset = calloc(BITSET_SIZE(res_num), sizeof(uint8_t));
    bitset->free_func = free_func;

    if (init_func == NULL || free_func == NULL || init_func(bitset) != LD_OK) {
        free(bitset->bitset);
        free(bitset);
        return NULL;
    }

    return bitset;
}

void free_bitset(void *v) {
    ld_bitset_t *bitset = v;
    if (bitset) {
        free(bitset->bitset);
        bitset->free_func(bitset->resources);
        free(bitset);
    }
}

l_err bs_record_by_index(ld_bitset_t *set, uint64_t i) {
    set->bitset[i / 8] |= (1 << (i % 8));
    return LD_OK;
}

l_err bs_alloc_resource(ld_bitset_t *set, void **res) {
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

bool bs_all_alloced(ld_bitset_t *set) {
    for (int i = 0; i < set->res_num; i++) {
        if ((set->bitset[i / 8] & (1 << (i % 8))) == 1) {
            return FALSE; // 发现有未分配的资源
        }
    }
    return TRUE;
}

int bs_get_alloced(ld_bitset_t *set) {
    int ret = 0;
    for (int i = 0; i < set->res_num; i++) {
        if ((set->bitset[i / 8] & (1 << (i % 8))) == 1) {
            ret++;
        }
    }
    return ret;
}

int bs_get_highest(ld_bitset_t *set) {
    int highest = 0;
    for (int i = 0; i < set->res_num; i++) {
        if ((set->bitset[i / 8] & (1 << (i % 8))) == 1) {
            highest = i;
        }
    }
    return highest;
}
