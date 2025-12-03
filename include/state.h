#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t calibrated;       // 0/1
    uint8_t pills_dispensed;  // 0..7
    uint8_t motor_active;     // 0/1 (flag set before rotation, cleared after)
    uint8_t reserved;
    uint32_t boot_count;
} DispenserState;

bool state_load(DispenserState* s);
bool state_save(const DispenserState* s);
bool state_reset(DispenserState* s);
