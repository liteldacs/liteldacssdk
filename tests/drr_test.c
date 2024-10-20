//
// Created by 邹嘉旭 on 2024/10/8.
//
#include "../global/ldacs_sim.h"
#include "ld_drr.h"

int main() {
    ld_drr_t *drr = init_ld_drr();

    ld_req_update(drr, 1, 100);
    ld_req_update(drr, 2, 200);
    ld_req_update(drr, 3, 900);
    ld_req_update(drr, 4, 900);

    size_t alloc_map_0[QUEUE_SIZE] = {0};

    resource_alloc(drr, 200, 1000, 0, alloc_map_0);


    for (int i = 0; i < 5; i++) {
        log_warn("\nSAC: %d ;\nALLOCED: %d ;\nDC: %d ; \nREMAIN_REQ: %d;\n",
                 drr->req_entitys[i].SAC,
                 alloc_map_0[i],
                 drr->req_entitys[i].DC,
                 drr->req_entitys[i].req_sz);
    }
    log_error("%d", ld_rbuffer_count(drr->active_list));

    size_t alloc_map_1[QUEUE_SIZE] = {0};
    resource_alloc(drr, 200, 1000, 0, alloc_map_1);

    for (int i = 0; i < 5; i++) {
        log_info("\nSAC: %d ;\nALLOCED: %d ;\nDC: %d ; \nREMAIN_REQ: %d;\n",
                 drr->req_entitys[i].SAC,
                 alloc_map_1[i],
                 drr->req_entitys[i].DC,
                 drr->req_entitys[i].req_sz);
    }

    ld_req_update(drr, 4, 1500);
    ld_req_update(drr, 0, 200);
    size_t alloc_map_2[QUEUE_SIZE] = {0};
    resource_alloc(drr, 200, 1000, 0, alloc_map_2);

    for (int i = 0; i < 5; i++) {
        log_fatal("\nSAC: %d ;\nALLOCED: %d ;\nDC: %d ; \nREMAIN_REQ: %d;\n",
                  drr->req_entitys[i].SAC,
                  alloc_map_2[i],
                  drr->req_entitys[i].DC,
                  drr->req_entitys[i].req_sz);
    }


    return 0;
}
