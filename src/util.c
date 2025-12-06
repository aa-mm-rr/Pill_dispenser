#include "pico/stdlib.h"
#include "util.h"

uint32_t now_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

void sleep_ms_blocking(uint32_t ms) {
    sleep_ms(ms);
}

bool wait_for_condition_ms(bool (*cond)(void), uint32_t timeout_ms) {
    uint32_t t0 = now_ms();
    while (now_ms() - t0 < timeout_ms) {
        if (cond()) return true;
        sleep_ms(1);
    }
    return false;
}
