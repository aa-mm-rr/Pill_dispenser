#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t boot_count;
    uint8_t  calibrated;
    uint8_t  motor_active;
    uint8_t  pills_dispensed;
    uint16_t steps_per_rev;
} DispenserState;

bool state_load(DispenserState* s);
void state_save(const DispenserState* s);
void state_reset(DispenserState* s);

#endif

