#ifndef STATE_H
#define STATE_H
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SYS_BOOT = 0,
    SYS_WAIT_CAL_BUTTON,
    SYS_CALIBRATING,
    SYS_READY_TO_START,
    SYS_DISPENSING,
    SYS_EMPTY,
} system_state_t;

typedef struct {
    uint32_t magic;
    uint16_t version;

    // Dispenser progress
    uint8_t current_slot;      // 0-7; 0 is calibration slot
    uint8_t dispenses_done;
    uint8_t pills_remaining;

    // Flags
    bool joined_network;       // LoRa join persisted
    bool motor_in_progress;    // true if power-off occurred mid-turn
    bool calibrated;           // wheel aligned to calibration slot

    // Logs (basic counters)
    uint32_t boots_count;
    uint32_t pills_dispensed_count;
    uint32_t pills_missed_count;

    // Timestamps just in case we use it
    uint32_t last_event_ms;

    // Reserved for future
    uint8_t reserved[32];
    uint16_t steps_per_slot; // dynamically calibrated

} nv_state_t;

extern nv_state_t g_state;

void state_init_defaults(void);
void state_load(void);
void state_save(void);

#endif
