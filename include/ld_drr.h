//
// Created by 邹嘉旭 on 2024/10/10.
//

#ifndef LD_DRR_H
#define LD_DRR_H
#include "../global/ldacs_sim.h"
#include "ld_rbuffer.h"
#include "ld_util_def.h"

typedef struct drr_req_s {
    uint16_t SAC;
    uint64_t DC;
    size_t req_sz;
} drr_req_t;

typedef struct ld_drr_s {
    ld_rbuffer *active_list;
    size_t req_szs[QUEUE_SIZE];

    drr_req_t req_entitys[QUEUE_SIZE];
} ld_drr_t;

ld_drr_t *init_ld_drr();

void ld_req_update(ld_drr_t *drr, uint16_t SAC, size_t req_sz);

l_err resource_alloc(ld_drr_t *drr, size_t pkt_size, size_t W, size_t W_min, size_t *alloc_map);


#endif //LD_DRR_H
