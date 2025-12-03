#include "util.h"
#include "pico/time.h"

uint32_t millis(void){
    return to_ms_since_boot(get_absolute_time());
}

void periodic_start(periodic_t* t, uint32_t now_ms, uint32_t period_ms){
    t->start_ms = now_ms;
    t->period_ms = period_ms;
}

bool periodic_due(periodic_t* t, uint32_t now_ms){
    if (now_ms - t->start_ms >= t->period_ms){
        t->start_ms += t->period_ms;
        return true;
    }
    return false;
}
