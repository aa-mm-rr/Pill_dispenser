#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "config.h"
#include "state.h"
#include "stepper.h"
#include "sensors.h"
#include "eeprom.h"
#include "leds.h"
#include "buttons.h"
#include "util.h"

extern nv_state_t g_state;

// Recovery after power loss
static void safe_recover_if_mid_turn(void) {
    if (g_state.motor_in_progress) {
        printf("(RECOVERY) Power loss detected mid-turn. Resuming without rotation.\n");
        g_state.motor_in_progress = false;
        state_save();
        printf("(RECOVERY) Resume from slot=%u, calibrated=%d, sps=%u\n",
               g_state.current_slot, g_state.calibrated, g_state.steps_per_slot);
    }
}

int main() {
    stdio_init_all();
    sensors_init();
    sleep_ms(2000);
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("Hello from Pill dispenser\n");

    leds_init();      printf("(INIT) LEDs initialized.\n");
    buttons_init();   printf("(INIT) Buttons initialized.\n");
    sensors_init();   printf("(INIT) Sensors initialized.\n");
    stepper_init();   printf("(INIT) Stepper initialized.\n");
    eeprom_init();    printf("(INIT) EEPROM initialized.\n");

    state_load();
    g_state.boots_count++;
    state_save();

    safe_recover_if_mid_turn();

    // System state machine
    system_state_t sys;
    if (g_state.calibrated) {
        if (g_state.dispenses_done < DISPENSE_SLOTS) {
            sys = SYS_DISPENSING;
            printf("(RECOVERY) Continuing dispensing from slot=%u\n", g_state.current_slot);
        } else {
            sys = SYS_READY_TO_START;
            printf("(RECOVERY) Cycle complete, ready to start new dispensing.\n");
        }
    } else {
        sys = SYS_WAIT_CAL_BUTTON;
        printf("(ACTION) Press CAL button to start wheel calibration.\n");
    }

    while (true) {
        switch (sys) {
            case SYS_WAIT_CAL_BUTTON: {
                leds_wait_blink();
                if (button_cal_pressed()) {
                    sys = SYS_CALIBRATING;
                    printf("(EVENT) Calibration button pressed.\n");
                }
                break;
            }

            case SYS_CALIBRATING: {
                stepper_mark_motion_begin();
                uint32_t rev1 = stepper_calibrate_revolution();
                uint32_t rev2 = stepper_calibrate_revolution();
                stepper_mark_motion_end();

                if (rev1 > 0 && rev2 > 0) {
                    uint32_t mean = (rev1 + rev2) / 2;
                    g_state.steps_per_slot = (uint16_t)(mean / TOTAL_COMPARTMENTS);
                    g_state.current_slot = CALIBRATION_SLOT_INDEX;
                    g_state.calibrated = true;
                    g_state.dispenses_done = 0;
                    g_state.pills_remaining = DISPENSE_SLOTS;
                    state_save();

                    printf("(SUCCESS) Calibration complete. Press button 2 to start dispensing.\n");
                    sys = SYS_READY_TO_START;
                } else {
                    leds_blink_error(5);
                    printf("(ERROR) Calibration failed.\n");
                    sys = SYS_WAIT_CAL_BUTTON;
                }
                break;
            }

            case SYS_READY_TO_START: {
                leds_on_ready();
                if (button_start_pressed()) {
                    printf("(EVENT) START button pressed.\n");
                    sys = SYS_DISPENSING;
                }
                break;
            }

            case SYS_DISPENSING: {
                while (g_state.dispenses_done < DISPENSE_SLOTS) {
                    printf("(INFO) Waiting %u seconds before next dispensing turn.\n",
                           DISPENSE_INTERVAL_MS / 1000);
                    uint32_t t0 = now_ms();
                    while (now_ms() - t0 < DISPENSE_INTERVAL_MS) {
                        tight_loop_contents();
                    }

                    // Show which pill is being dispensed
                    printf("(MOTION) Dispensing pill number %u...\n", g_state.dispenses_done + 1);

                    stepper_mark_motion_begin();
                    g_state.motor_in_progress = true;
                    state_save();

                    for (uint32_t i = 0; i < g_state.steps_per_slot; ++i) {
                        stepper_step_sequence_once();
                        sleep_us(STEPPER_STEP_DELAY_US);
                    }

                    stepper_mark_motion_end();
                    g_state.motor_in_progress = false;
                    slot_advance();
                    state_save();

                    printf("(SENSOR) Waiting for pill hit...\n");
                    uint32_t t1 = now_ms();
                    bool hit = false;
                    while (now_ms() - t1 < PIEZO_FALL_WINDOW_MS) {
                        if (piezo_was_triggered()) {
                            hit = true;
                            break;
                        }
                    }
                    piezo_reset_flag();

                    g_state.dispenses_done++;
                    if (hit) {
                        g_state.pills_dispensed_count++;
                        if (g_state.pills_remaining > 0) g_state.pills_remaining--;
                        leds_dispense_progress(g_state.dispenses_done);
                        state_save();
                        printf("(SUCCESS) Pill %u detected.\n", g_state.dispenses_done);
                    } else {
                        g_state.pills_missed_count++;
                        leds_blink_error(5);
                        state_save();
                        printf("(WARNING) Pill %u not detected.\n", g_state.dispenses_done);
                    }
                }


                printf("(INFO) One full circle complete. Dispenser empty.\n");
                g_state.calibrated = false;
                g_state.dispenses_done = 0;
                g_state.pills_remaining = DISPENSE_SLOTS;
                state_save();
                sys = SYS_EMPTY;
                break;
            }

            case SYS_EMPTY: {
                printf("(INFO) All pills dispensed. Press button 1 to restart.\n");
                g_state.calibrated = false;
                state_save();
                sys = SYS_WAIT_CAL_BUTTON;
                break;
            }
        }
        sleep_ms(10);
    }
    return 0;
}
