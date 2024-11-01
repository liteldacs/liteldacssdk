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
    size_t max_sz;
    // size_t req_szs[QUEUE_SIZE];
    //
    // drr_req_t req_entitys[QUEUE_SIZE];

    size_t *req_szs;

    drr_req_t *req_entitys;
} ld_drr_t;

typedef void (*alloc_cb)(ld_drr_t *drr, size_t *);

ld_drr_t *init_ld_drr(size_t sz);

l_err free_ld_drr(ld_drr_t *drr);

void ld_req_update(ld_drr_t *drr, uint16_t SAC, size_t req_sz);

l_err drr_resource_alloc(ld_drr_t *drr, size_t pkt_size, size_t W, size_t W_min, alloc_cb cb);


#endif //LD_DRR_H
