//
// Created by 邹嘉旭 on 2025/4/3.
//

#ifndef LD_PRIMITIVE_H
#define LD_PRIMITIVE_H
#include "ld_log.h"
#include "ldacs_sim.h"
#include "ld_thread.h"

typedef struct ld_prim_s ld_prim_t;
typedef void (*prim_func)(ld_prim_t *);

struct ld_prim_s {
    const char *name;
    uint16_t prim_seq;
    prim_func SAP[3];
    prim_func req_cb[3];
    uint8_t prim_obj_typ;
    void *prim_objs;
    l_err prim_err;
    pthread_mutex_t mutex;
};


l_err preempt_prim(ld_prim_t *prim, uint8_t ele_typ, void *data, void (*free_func)(void *), uint8_t sap_ind,
                   uint32_t cb_ind);

void *dup_prim_data(void *data, size_t size);

void clear_dup_prim_data(void *data, void (*free_func)(void *));

#endif //LD_PRIMITIVE_H
