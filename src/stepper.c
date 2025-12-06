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

/* Debounced sensor read: vote over small samples to stabilize noisy opto */
static bool opto_read_stable(void) {
    int low = 0, high = 0;
    for (int i = 0; i < 8; ++i) {
        bool v = opto_is_opening_at_sensor();
        if (v) high++; else low++;
        sleep_us(300);
    }
    return high > low;
}

/* Fast helpers using raw reads (no debounce) to avoid missing narrow holes */
static inline bool opto_raw_open(void)   { return opto_is_opening_at_sensor(); }
static inline bool opto_raw_closed(void) { return !opto_is_opening_at_sensor(); }

/* Seek until sensor reports OPEN using raw reads, then confirm with debounce */
static bool seek_open_then_confirm(uint32_t max_steps) {
    for (uint32_t i = 0; i < max_steps; ++i) {
        stepper_step_sequence_once();
        sleep_us(STEPPER_STEP_DELAY_US);
        if (opto_raw_open()) {
            // Confirm with stable read
            if (opto_read_stable()) return true;
        }
    }
    return false;
}

/* Seek until sensor reports CLOSED using raw reads, then confirm with debounce */
static bool seek_closed_then_confirm(uint32_t max_steps) {
    for (uint32_t i = 0; i < max_steps; ++i) {
        stepper_step_sequence_once();
        sleep_us(STEPPER_STEP_DELAY_US);
        if (opto_raw_closed()) {
            if (!opto_read_stable()) return true;
        }
    }
    return false;
}

/* Count one revolution: leave hole → next hole → leave hole */
uint32_t stepper_calibrate_revolution(void) {
    printf("[CAL] Seeking first hole...\n");

    // Seek OPEN (hole) anywhere within 2 nominal turns
    if (!seek_open_then_confirm(NOMINAL_FULL_REV_STEPS * 2)) {
        printf("[CAL][ERR] Failed to find initial OPEN within timeout.\n");
        return 0;
    }

    // Leave hole to CLOSED (edge right after hole)
    if (!seek_closed_then_confirm(NOMINAL_FULL_REV_STEPS / 2)) {
        printf("[CAL][ERR] Failed to leave hole to CLOSED.\n");
        return 0;
    }

    uint32_t steps = 0;

    // Count steps until we hit the next hole (OPEN)
    while (steps < NOMINAL_FULL_REV_STEPS * 2) {
        stepper_step_sequence_once();
        sleep_us(STEPPER_STEP_DELAY_US);
        steps++;
        if (opto_raw_open() && opto_read_stable()) {
            // Now leave the hole (CLOSED) to finish the revolution at the end of hole
            while (steps < NOMINAL_FULL_REV_STEPS * 2) {
                stepper_step_sequence_once();
                sleep_us(STEPPER_STEP_DELAY_US);
                steps++;
                if (opto_raw_closed() && !opto_read_stable()) {
                    printf("[CAL] Revolution complete at end of hole. Steps=%u\n", steps);
                    return steps;
                }
            }
            printf("[CAL][ERR] Timeout leaving hole after OPEN.\n");
            return 0;
        }
    }

    printf("[CAL][ERR] Timeout seeking next OPEN.\n");
    return 0;
}

/* Count two consecutive revolutions */
bool calibrate_two_revolutions(uint32_t *rev1_steps, uint32_t *rev2_steps) {
    *rev1_steps = stepper_calibrate_revolution();
    if (*rev1_steps == 0) return false;

    *rev2_steps = stepper_calibrate_revolution();
    if (*rev2_steps == 0) return false;

    return true;
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
