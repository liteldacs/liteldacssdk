//
// Created by 邹嘉旭 on 2024/10/10.
//
#include "ld_drr.h"

ld_drr_t *init_ld_drr(size_t sz) {
    ld_drr_t *drr = calloc(1, sizeof(ld_drr_t));
    drr->active_list = ld_rbuffer_init(sz);
    drr->max_sz = sz;
    drr->req_szs = calloc(sz, sizeof(size_t));
    drr->req_entitys = calloc(sz, sizeof(drr_req_t));
    zero(drr->req_szs);

    for (int i = 0; i < sz; i++) {
        drr->req_entitys[i].SAC = i;
        drr->req_entitys[i].DC = 0;
    }

    return drr;
}

l_err free_ld_drr(ld_drr_t *drr) {
    if (drr) {
        if (drr->active_list)   ld_rbuffer_free(drr->active_list);
        if (drr->req_szs)   free(drr->req_szs);
        if (drr->req_entitys)   free(drr->req_entitys);
        free(drr);
        return LD_OK;
    }
    return LD_ERR_NULL;
}

void ld_req_update(ld_drr_t *drr, uint16_t SAC, size_t req_sz) {
    drr->req_entitys[SAC].req_sz += req_sz;
}


static bool drr_fac(const void *a, const void *b) {
    if (((drr_req_t *) a)->SAC == ((drr_req_t *) b)->SAC) return TRUE;
    return FALSE;
}

/**
* LDACS Default Resource Allocation.(DWRR / DRR)
*
*
* W: The total remaining available resources.
* W_min: The minimum threshold for resources.
* DC_i: The deficit counter for the currently served user.
* pkt_size: The size of a fixed packet for each transmission.
* ReqMap_i: The data amount requested by the user.
* AllocMap_i: The allocated data amount for the user.
* activelist: A queue of active users awaiting bandwidth.
*/
l_err drr_resource_alloc(ld_drr_t *drr, size_t pkt_size, size_t W, size_t W_min, alloc_cb callback_func, void *cb_args) {
    size_t total_req_bytes = 0;
    size_t *alloc_map = calloc(drr->max_sz, sizeof(size_t));
    for (int i = 0; i < drr->max_sz; i++) {
        drr_req_t *req_i = &drr->req_entitys[i];
        if (req_i->req_sz == 0) continue;

        /* Addition of a user to active list */
        if (ld_rbuffer_check_exist(drr->active_list, req_i, drr_fac) == FALSE) {
            ld_rbuffer_push_back(drr->active_list, req_i);
            req_i->DC = 0;
        }
        drr->req_szs[req_i->SAC] = req_i->req_sz;
        total_req_bytes += req_i->req_sz;
    }

    /* Underloaded case */
    if (total_req_bytes < W) W = total_req_bytes;

    while (W > W_min) {
        drr_req_t *req_i = NULL;
        bool frag_data = FALSE;
        ld_rbuffer_pop(drr->active_list, (void **) &req_i);
        if (req_i == NULL) break;

        req_i->DC += pkt_size;

        while (req_i->DC > 0 && req_i->req_sz > 0) {
            if (W >= pkt_size && req_i->DC >= pkt_size) {
                /* Give full packet size data */
                if (req_i->req_sz >= pkt_size) {
                    alloc_map[req_i->SAC] += pkt_size;
                    req_i->DC -= pkt_size;
                    W -= pkt_size;
                    req_i->req_sz -= pkt_size;
                } /* Give requested data */
                else if (req_i->req_sz < pkt_size) {
                    alloc_map[req_i->SAC] += req_i->req_sz;
                    req_i->DC -= req_i->req_sz;
                    W -= req_i->req_sz;
                    req_i->req_sz = 0;
                }
            } else if (W < pkt_size && W > W_min && req_i->DC >= pkt_size) {
                /* Give remaining data */
                if (req_i->req_sz >= W) {
                    alloc_map[req_i->SAC] += W;
                    req_i->req_sz -= W;
                    req_i->DC -= W;
                    W = 0;
                    frag_data = TRUE;
                } /* Give requested data */
                else if (req_i->req_sz < W) {
                    alloc_map[req_i->SAC] += req_i->req_sz;
                    req_i->DC -= req_i->req_sz;
                    W -= req_i->req_sz;
                    req_i->req_sz = 0;
                }
            } else {
                break;
            }
        }

        if (alloc_map[req_i->SAC] == drr->req_szs[req_i->SAC]) req_i->DC = 0;
        else if (frag_data == TRUE) ld_rbuffer_push_front(drr->active_list, req_i);
        else ld_rbuffer_push_back(drr->active_list, req_i);
    }
    callback_func(drr, alloc_map, cb_args);
    free(alloc_map);
    return LD_OK;
}
