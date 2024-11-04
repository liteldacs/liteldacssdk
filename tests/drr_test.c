//
// Created by 邹嘉旭 on 2024/10/8.
//
#include "../global/ldacs_sim.h"
#include "ld_drr.h"

void cb1(ld_drr_t *drr, size_t *alloc_map, void *args) {
    for (int i = 0; i < 5; i++) {
        log_warn("\nSAC: %d ;\nALLOCED: %d ;\nDC: %d ; \nREMAIN_REQ: %d;\n",
                 drr->req_entitys[i].SAC,
                 alloc_map[i],
                 drr->req_entitys[i].DC,
                 drr->req_entitys[i].req_sz);
    }
    log_error("%d", ld_rbuffer_count(drr->active_list));
}

void cb2(ld_drr_t *drr, size_t *alloc_map, void *args) {
    for (int i = 0; i < 5; i++) {
        log_info("\nSAC: %d ;\nALLOCED: %d ;\nDC: %d ; \nREMAIN_REQ: %d;\n",
                 drr->req_entitys[i].SAC,
                 alloc_map[i],
                 drr->req_entitys[i].DC,
                 drr->req_entitys[i].req_sz);
    }
    log_error("%d", ld_rbuffer_count(drr->active_list));
}
void cb3(ld_drr_t *drr, size_t *alloc_map, void *args) {
    for (int i = 0; i < 5; i++) {
        log_fatal("\nSAC: %d ;\nALLOCED: %d ;\nDC: %d ; \nREMAIN_REQ: %d;\n",
                 drr->req_entitys[i].SAC,
                 alloc_map[i],
                 drr->req_entitys[i].DC,
                 drr->req_entitys[i].req_sz);
    }
    log_error("%d", ld_rbuffer_count(drr->active_list));
}


int main() {
    ld_drr_t *drr = init_ld_drr(4096);

    ld_req_update(drr, 1, 100);
    ld_req_update(drr, 2, 200);
    ld_req_update(drr, 3, 900);
    ld_req_update(drr, 4, 900);

    drr_resource_alloc(drr, 200, 1000, 0, cb1, NULL);


    drr_resource_alloc(drr, 200, 1000, 0, cb2, NULL);


    ld_req_update(drr, 4, 1500);
    ld_req_update(drr, 0, 200);
    drr_resource_alloc(drr, 200, 1000, 0, cb3, NULL);

    // free_ld_drr(drr);

    return 0;
}
