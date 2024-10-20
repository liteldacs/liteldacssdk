//
// Created by 邹嘉旭 on 2024/2/19.
//

#include "ld_thread.h"

l_err ld_lock(pthread_mutex_t *mutex) {
    if (pthread_mutex_lock(mutex) != 0)
        return LD_ERR_LOCK;
    return LD_OK;
}

l_err ld_unlock(pthread_mutex_t *mutex) {
    if (0 != pthread_mutex_unlock(mutex))
        return LD_ERR_LOCK;
    return LD_OK;
}
