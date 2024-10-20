//
// Created by 邹嘉旭 on 2023/11/28.
//

#ifndef LDACS_SIM_UTIL_BASE64_H
#define LDACS_SIM_UTIL_BASE64_H


#include "../global/ldacs_sim.h"
#include "ld_buffer.h"
#include "ld_log.h"

l_err decode_base64(int flags, const char *src, const size_t src_len, char *dst, size_t *dst_len);

buffer_t *encode_b64_buffer(int flags, uint8_t *in_str, size_t handle_len);

buffer_t *decode_b64_buffer(int flags, uint8_t *in_str, size_t handle_len);


#endif //LDACS_SIM_UTIL_BASE64_H
