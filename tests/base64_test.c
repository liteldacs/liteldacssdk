//
// Created by 邹嘉旭 on 2024/2/24.
//
#include "../global/ldacs_sim.h"
#include <ld_buffer.h>
#include <ld_log.h>
#include "ld_base64.h"

int main() {
    const char *str =
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    buffer_t *bufs = decode_b64_buffer(0, str, 188);

    log_buf(LOG_WARN, "DECODE", bufs->ptr, bufs->len);
}
