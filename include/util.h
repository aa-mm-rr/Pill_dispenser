#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t start_ms;
    uint32_t period_ms;
} periodic_t;

void periodic_start(periodic_t* t, uint32_t now_ms, uint32_t period_ms);
bool periodic_due(periodic_t* t, uint32_t now_ms);
uint32_t millis(void);
