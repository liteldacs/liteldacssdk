//
// Created by 邹嘉旭 on 2024/2/19.
//

#ifndef LDACS_SIM_LD_THREAD_H
#define LDACS_SIM_LD_THREAD_H

#include "../global/ldacs_sim.h"

typedef struct safe_bigint_s {
    pthread_mutex_t mutex;
    uint64_t value;
} safe_bigint_t;

l_err ld_lock(pthread_mutex_t *mutex);

#define LD_LOCK(mutex){ \
    pthread_mutex_lock(mutex)\
}

l_err ld_unlock(pthread_mutex_t *mutex);

#endif //LDACS_SIM_LD_THREAD_H
