#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "config.h"
#include "state.h"
#include "stepper.h"
#include "util.h"
#include "sensors.h"
#include "eeprom.h"

static const uint8_t seq_halfstep[8] = {
    0b0001, // A
    0b0011, // A+B
    0b0010, // B
    0b0110, // B+C
    0b0100, // C
    0b1100, // C+D
    0b1000, // D
    0b1001  // D+A
};
static int seq_index = 0;

static void apply_mask(uint8_t mask) {
    gpio_put(PIN_STEPPER_IN1, (mask & 0x1) ? 1 : 0);
    gpio_put(PIN_STEPPER_IN2, (mask & 0x2) ? 1 : 0);
    gpio_put(PIN_STEPPER_IN3, (mask & 0x4) ? 1 : 0);
    gpio_put(PIN_STEPPER_IN4, (mask & 0x8) ? 1 : 0);
}

void stepper_init(void) {
    gpio_init(PIN_STEPPER_IN1);
    gpio_init(PIN_STEPPER_IN2);
    gpio_init(PIN_STEPPER_IN3);
    gpio_init(PIN_STEPPER_IN4);
    gpio_set_dir(PIN_STEPPER_IN1, GPIO_OUT);
    gpio_set_dir(PIN_STEPPER_IN2, GPIO_OUT);
    gpio_set_dir(PIN_STEPPER_IN3, GPIO_OUT);
    gpio_set_dir(PIN_STEPPER_IN4, GPIO_OUT);
    apply_mask(0);
    printf("[STEPPER] ULN2003A half-stepping sequence configured.\n");
}

void stepper_mark_motion_begin(void) {
    printf("[STEPPER] Motion begin. Persisting flag for power-loss detection.\n");
    g_state.motor_in_progress = true;
    state_save();
}

void stepper_mark_motion_end(void) {
    printf("[STEPPER] Motion end. Clearing power-loss flag and persisting.\n");
    g_state.motor_in_progress = false;
    state_save();
}

void stepper_step_sequence_once(void) {
    apply_mask(seq_halfstep[seq_index]);
    seq_index = (seq_index + 1) % 8;
}

void stepper_steps(uint32_t steps) {
    for (uint32_t i = 0; i < steps; ++i) {
        stepper_step_sequence_once();
        sleep_us(STEPPER_STEP_DELAY_US);
    }
    apply_mask(0); // optional de-energize
}

void stepper_advance_one_slot(void) {
    printf("[STEPPER] Advancing one slot (%u steps)...\n", g_state.steps_per_slot);
    stepper_steps(g_state.steps_per_slot);
}

void stepper_full_turn_nominal(void) {
    printf("[STEPPER] Performing nominal full turn (%u steps)...\n", NOMINAL_FULL_REV_STEPS);
    stepper_steps(NOMINAL_FULL_REV_STEPS);
}

// Debounced sensor read
static bool opto_read_stable(void) {
    int low = 0, high = 0;
    for (int i = 0; i < 8; ++i) {
        bool v = opto_is_opening_at_sensor();
        if (v) high++; else low++;
        sleep_us(300);
    }
    return high > low;
}

uint32_t stepper_calibrate_revolution(void) {
    printf("[CAL] Phase 1: Seeking first OPEN (hole at sensor)...\n");
    bool open = false;
    for (uint32_t i = 0; i < NOMINAL_FULL_REV_STEPS * 2; ++i) {
        stepper_step_sequence_once();
        sleep_us(STEPPER_STEP_DELAY_US);
        open = opto_read_stable();
        if (open) {
            printf("[CAL] Hole detected (OPEN). Proceeding to leave the hole.\n");
            break;
        }
    }
    if (!open) {
        printf("[CAL] Failed to detect initial OPEN within scan window.\n");
        return 0;
    }

    printf("[CAL] Phase 2: Leaving hole (waiting for CLOSED)...\n");
    bool closed = false;
    for (uint32_t i = 0; i < NOMINAL_FULL_REV_STEPS / 2; ++i) {
        stepper_step_sequence_once();
        sleep_us(STEPPER_STEP_DELAY_US);
        closed = !opto_read_stable();
        if (closed) {
            printf("[CAL] Hole left (CLOSED). Starting revolution step count.\n");
            break;
        }
    }
    if (!closed) {
        printf("[CAL] Failed to leave hole (never saw CLOSED). Check sensor polarity/width.\n");
        return 0;
    }

    printf("[CAL] Phase 3: Counting steps until next OPEN...\n");
    uint32_t steps = 0;
    bool next_open = false;
    for (uint32_t i = 0; i < NOMINAL_FULL_REV_STEPS * 2; ++i) {
        stepper_step_sequence_once();
        sleep_us(STEPPER_STEP_DELAY_US);
        steps++;
        next_open = opto_read_stable();
        if (next_open) {
            printf("[CAL] Next hole detected (OPEN). Revolution steps = %u\n", steps);
            return steps;
        }
    }

    printf("[CAL] Did not detect next OPEN within scan window. Calibration failed.\n");
    return 0;
}

static uint8_t current_slot = CALIBRATION_SLOT_INDEX;

void slot_set(uint8_t slot_index) {
    current_slot = slot_index % TOTAL_COMPARTMENTS;
    printf("[SLOT] Logical slot set to %u.\n", current_slot);
}

void slot_advance(void) {
    current_slot = (current_slot + 1) % TOTAL_COMPARTMENTS;
    g_state.current_slot = current_slot;
    state_save();
    printf("[SLOT] Logical slot advanced. Current=%u.\n", current_slot);
}

uint8_t slot_get(void) {
    return current_slot;
}
