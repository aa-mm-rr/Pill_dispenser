#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#include <stdbool.h>

uint32_t now_ms(void);
void sleep_ms_blocking(uint32_t ms);
bool wait_for_condition_ms(bool (*cond)(void), uint32_t timeout_ms);

#endif
