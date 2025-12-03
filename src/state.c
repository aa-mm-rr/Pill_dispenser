
#include "state.h"
#include "eeprom_at24c256.h"
#include <string.h>

#define STATE_ADDR 0x0000

bool state_load(DispenserState* s) {
    uint8_t buf[sizeof(DispenserState)];
    if (!eeprom_read(STATE_ADDR, buf, sizeof(buf))) return false;
    memcpy(s, buf, sizeof(*s));
    return true;
}
bool state_save(const DispenserState* s) {
    return eeprom_write(STATE_ADDR, (const uint8_t*)s, sizeof(*s));
}
bool state_reset(DispenserState* s) {
    memset(s, 0, sizeof(*s));
    return state_save(s);
}
