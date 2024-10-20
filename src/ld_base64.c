//
// Created by 邹嘉旭 on 2023/11/28.
//

#include "ld_base64.h"

l_err decode_base64(int flags, const char *src, const size_t src_len, char *dst, size_t *dst_len) {
    if (!base64_decode(src, src_len, dst, dst_len, flags)) {
        log_error("FAIL: decoding of '%s': decoding error\n", src);
        return LD_ERR_INTERNAL;
    }
    return LD_OK;
}

l_err encode_base64(int flags, const char *src, const size_t src_len, char *dst, size_t *dst_len) {
    base64_encode(src, src_len, dst, dst_len, flags);
    return LD_OK;
}

buffer_t *encode_b64_buffer(int flags, uint8_t *in_str, size_t handle_len) {
    buffer_t *buf = init_buffer_unptr();
    if (!buf) return NULL;

    uint8_t dst[MAX_JSON_ENBASE64] = {0};
    size_t dst_len = 0;
    encode_base64(flags, (char *) in_str, handle_len, (char *) dst, &dst_len);

    CLONE_TO_CHUNK(*buf, dst, dst_len)
    return buf;
}

buffer_t *decode_b64_buffer(int flags, uint8_t *in_str, size_t handle_len) {
    buffer_t *buf = init_buffer_unptr();
    if (!buf) {
        log_warn("To decode buf is Empty");
        return NULL;
    }

    uint8_t dst[MAX_JSON_DEBASE64] = {0};
    size_t dst_len = 0;
    if (decode_base64(flags, (char *) in_str, handle_len, (char *) dst, &dst_len)) {
        log_error("Cannot decode base64");
        free_buffer(buf);
        return NULL;
    };

    CLONE_TO_CHUNK(*buf, dst, dst_len)
    return buf;
}
