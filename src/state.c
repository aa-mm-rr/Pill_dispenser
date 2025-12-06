#include <string.h>
#include "pico/stdlib.h"
#include "config.h"
#include "state.h"
#include "eeprom.h"

nv_state_t g_state;

void state_init_defaults(void) {
    memset(&g_state, 0, sizeof(g_state));
    g_state.magic = STATE_MAGIC;
    g_state.version = STATE_VERSION;
    g_state.current_slot = CALIBRATION_SLOT_INDEX;
    g_state.dispenses_done = 0;
    g_state.pills_remaining = DISPENSE_SLOTS;
    g_state.joined_network = false;
    g_state.motor_in_progress = false;
    g_state.calibrated = false;
    g_state.steps_per_slot = 512; // default fallback

}

void state_load(void) {
    uint8_t buf[EEPROM_STATE_SIZE];
    if (!eeprom_read(EEPROM_STATE_ADDR, buf, sizeof(buf))) {
        state_init_defaults();
        return;
    }
    memcpy(&g_state, buf, sizeof(nv_state_t));
    if (g_state.magic != STATE_MAGIC || g_state.version != STATE_VERSION) {
        state_init_defaults();
    }
}

void state_save(void) {
    uint8_t buf[EEPROM_STATE_SIZE];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &g_state, sizeof(nv_state_t));
    eeprom_write(EEPROM_STATE_ADDR, buf, sizeof(buf));
}
