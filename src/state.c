#include "state.h"
#include "eeprom_at24c256.h"
#include "board.h"
#include <string.h>

#define STATE_ADDR 0x0000

bool state_load(DispenserState* s) {
    if (!eeprom_read(STATE_ADDR, (uint8_t*)s, sizeof(DispenserState))) {
        return false;
    }
    return true;
}

void state_save(const DispenserState* s) {
    eeprom_write(STATE_ADDR, (const uint8_t*)s, sizeof(DispenserState));
}

void state_reset(DispenserState* s) {
    memset(s, 0, sizeof(DispenserState));
    s->steps_per_rev = CALIBRATION_FULL_STEPS; // default 4096
}
