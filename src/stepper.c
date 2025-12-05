#include "stepper.h"
#include "board.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "sensors.h"
#include "state.h"
#include <stdio.h>

// Half-step sequence for 28BYJ-48 (IN1=A, IN2=B, IN3=C, IN4=D)
static const uint8_t halfstep_seq[8][4] = {
    {1,0,0,0}, // A
    {1,1,0,0}, // A+B
    {0,1,0,0}, // B
    {0,1,1,0}, // B+C
    {0,0,1,0}, // C
    {0,0,1,1}, // C+D
    {0,0,0,1}, // D
    {1,0,0,1}  // A+D
};

static int phase_idx = 0;

static inline void drive_halfstep(int phase) {
    gpio_put(PIN_ST_IN1, halfstep_seq[phase][0]);
    gpio_put(PIN_ST_IN2, halfstep_seq[phase][1]);
    gpio_put(PIN_ST_IN3, halfstep_seq[phase][2]);
    gpio_put(PIN_ST_IN4, halfstep_seq[phase][3]);
}

static inline void step_once(void) {
    phase_idx = (phase_idx + 1) & 7;
    drive_halfstep(phase_idx);
    sleep_ms(3);
}

static inline void motor_stop(void) {
    gpio_put(PIN_ST_IN1, 0);
    gpio_put(PIN_ST_IN2, 0);
    gpio_put(PIN_ST_IN3, 0);
    gpio_put(PIN_ST_IN4, 0);
}

void stepper_init(void) {
    gpio_init(PIN_ST_IN1); gpio_set_dir(PIN_ST_IN1, true);
    gpio_init(PIN_ST_IN2); gpio_set_dir(PIN_ST_IN2, true);
    gpio_init(PIN_ST_IN3); gpio_set_dir(PIN_ST_IN3, true);
    gpio_init(PIN_ST_IN4); gpio_set_dir(PIN_ST_IN4, true);
    phase_idx = 0;
    drive_halfstep(phase_idx);
}

void stepper_set_motor_active(bool active) {
    DispenserState s;
    if (!state_load(&s)) return;
    s.motor_active = active ? 1 : 0;
    state_save(&s);
}

bool stepper_rotate_steps(int steps) {
    if (steps <= 0) return true;
    stepper_set_motor_active(true);
    for (int i = 0; i < steps; i++) step_once();
    motor_stop();
    stepper_set_motor_active(false);
    return true;
}


// Rotate slot while checking piezo sensor for pill drop
bool stepper_rotate_and_check(int steps, uint32_t window_ms) {
    stepper_set_motor_active(true);
    absolute_time_t deadline = make_timeout_time_ms(window_ms);
    bool detected = false;

    for (int i = 0; i < steps; i++) {
        // advance motor one half-step
        phase_idx = (phase_idx + 1) & 7;
        drive_halfstep(phase_idx);
        sleep_ms(3);

        // sample piezo during rotation (fast sampling)
        int now = gpio_get(PIN_PIEZO);
        if (now == 0) {
            detected = true;
            // optional: break early if pill detected
        }

    }

    motor_stop();
    stepper_set_motor_active(false);
    return detected;
}

// --- Calibration logic unchanged except saving average ---
static  int opto_read(void) { return sensors_opto_read(); }
static  bool opto_inside_hole(void)  { return opto_read() == 0; }
static bool opto_outside_hole(void) { return opto_read() == 1; }

static bool wait_until_outside_hole(int* steps, int limit) {
    while (opto_inside_hole() && *steps < limit) {
        step_once();
        (*steps)++;
    }
    return opto_outside_hole();
}

static bool wait_for_end_of_hole(int* steps, int limit) {
    int prev = opto_read();
    while (*steps < limit) {
        step_once();
        (*steps)++;
        int now = opto_read();
        if (prev == 0 && now == 1) {
            return true; // end-of-hole found
        }
        prev = now;
    }
    return false;
}

bool stepper_calibrate_blocking(void) {
    printf("Starting calibration (3 revs, end-of-hole)...\n");

    const int steps_per_rev_nominal = 4096;
    const int max_steps = steps_per_rev_nominal * 5;
    int steps = 0;

    stepper_set_motor_active(true);

    if (!wait_until_outside_hole(&steps, max_steps)) {
        printf("Could not exit hole before starting. Steps=%d\n", steps);
        motor_stop();
        stepper_set_motor_active(false);
        return false;
    }

    if (!wait_for_end_of_hole(&steps, max_steps)) {
        printf("No initial end-of-hole found. Steps=%d\n", steps);
        motor_stop();
        stepper_set_motor_active(false);
        return false;
    }
    int ref0 = steps;
    printf("Ref 0 at %d\n", ref0);

    int refs[3];
    for (int i = 0; i < 3; i++) {
        int target = ref0 + (i + 1) * steps_per_rev_nominal - 64;
        while (steps < target && steps < max_steps) {
            step_once();
            steps++;
            if (steps % 512 == 0) printf("Progress: %d steps...\n", steps);
        }
        if (!wait_for_end_of_hole(&steps, max_steps)) {
            printf("Could not find end-of-hole #%d. Steps=%d\n", i + 1, steps);
            motor_stop();
            stepper_set_motor_active(false);
            return false;
        }
        refs[i] = steps;
        printf("Ref %d at %d\n", i + 1, refs[i]);
    }

    int d1 = refs[0] - ref0;
    int d2 = refs[1] - refs[0];
    int d3 = refs[2] - refs[1];
    int avg = (d1 + d2 + d3) / 3;

    printf("Average steps per revolution: %d\n", avg);

    DispenserState s;
    if (state_load(&s)) {
        s.steps_per_rev = (uint16_t)avg;
        state_save(&s);
        printf("Stored steps_per_rev=%u into EEPROM state.\n", s.steps_per_rev);
    }

    motor_stop();
    stepper_set_motor_active(false);
    return true;
}
