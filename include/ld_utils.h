//
// Created by 邹嘉旭 on 2024/10/17.
//

#ifndef LD_UTILS_H
#define LD_UTILS_H

#include "../global/ldacs_sim.h"
#include "ld_buffer.h"
#include "ld_util_def.h"

void get_time(char *time_str, enum TIME_MOD t_mod, enum TIME_PREC t_pesc);

int32_t memrchr(const buffer_t *buf, uint8_t target);

int bytes_len(size_t n);

#define UA_STR(name) char (name)[11] = {0}

static char *get_ua_str(uint32_t value, char *str) {
    if (value >= (1 << UALEN)) return NULL;
    sprintf(str, "%09d", value);
    return str;
}

int ld_split(const char *str, char ***argv);



#endif //LD_UTILS_H
