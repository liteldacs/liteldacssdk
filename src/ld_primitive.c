//
// Created by 邹嘉旭 on 2025/4/3.
//
#include "ld_primitive.h"
static l_err prim_lock(ld_prim_t *prim_p) {
    if (prim_p == NULL)
        return LD_ERR_INVALID;
    return ld_lock(&prim_p->mutex);
}

static l_err prim_unlock(ld_prim_t *prim_p) {
    if (prim_p == NULL)
        return LD_ERR_INVALID;
    return ld_unlock(&prim_p->mutex);
}


/**
 * 有SAP和callback 为上层申请的REQ原语
 * 有SAP无callback 为上层申请的IND原语
 * 无SAP有callback 为下层申请的IND原语
 * @param prim  primitive struct address
 * @param data  primitive context data
 * @param free_func     the function which is used to free data
 */
l_err preempt_prim(ld_prim_t *prim, uint8_t ele_typ, void *data, void (*free_func)(void *),
                   uint8_t sap_ind, uint32_t cb_ind) {
    l_err err;
    do {
        if (prim_lock(prim)) {
            err = LD_ERR_LOCK;
            break;
        }
        prim->prim_objs = data;
        prim->prim_obj_typ = ele_typ;

        if (prim->SAP[sap_ind]) {
            prim->SAP[sap_ind](prim);
        }
        if (prim->req_cb[cb_ind]) {
            prim->req_cb[cb_ind](prim);
        }

        err = prim->prim_err; /* record the prim_err, pretending to return */

        if (prim->prim_objs && free_func) {
            free_func(prim->prim_objs);
        }
        prim->prim_objs = NULL;
        prim->prim_obj_typ = 0xFF;
        prim->prim_err = LD_OK;
        if (prim_unlock(prim) != LD_OK) {
            err = LD_ERR_LOCK;
        }
    } while (0);

    return err;
}

/**
 * copy the obj memory, to avoid the memory leaks caused by using elements
 * in obj outside of the primitive.
 * @param data
 * @param size
 * @return
 */
void *dup_prim_data(void *data, size_t size) {
    void *ret = malloc(size);
    memcpy(ret, data, size);
    return ret;
}

void clear_dup_prim_data(void *data, void (*free_func)(void *)) {
    if (data) {
        free_func(data);
    }
}

