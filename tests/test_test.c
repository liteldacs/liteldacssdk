
#include <ld_log.h>

#include "ld_bitset.h"

int main() {
    uint8_t a[4] = {0};
    a[0] = 0x04;
    a[1] = 0x01;

    size_t ret = 0;
    for (int i = 0; i < 32; i++) {
        log_error("%d %d %d %d", i, a[i/8], (1 << (i % 8)), a[i / 8] & (1 << (i % 8)));
        if ((a[i / 8] & (1 << (i % 8))) != 0) {
            ret++;
        }
    }
    log_warn("%d", ret);

    // int highest = -1;
    // for (int i = 0; i < 32; i++) {
    //     if ((a[i / 8] & (1 << (i % 8))) != 0) {
    //         highest = i;
    //     }
    // }
    // log_warn("%d", highest);
}
