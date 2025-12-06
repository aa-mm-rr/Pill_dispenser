#include <stdio.h>
#include "pico/stdlib.h"
#include "config.h"
#include "state.h"
#include "stepper.h"
#include "sensors.h"
#include "eeprom.h"
#include "lora.h"
#include "leds.h"
#include "buttons.h"
#include "util.h"

// LoRa APPKEY
static const char *LORA_APPKEY = "00112233445566778899AABBCCDDEEFF";

extern nv_state_t g_state;

static void report_status(const char *event) {
    char msg[160];
    snprintf(msg, sizeof(msg),
             "event=%s;dispenses=%u;miss=%u;slot=%u;pills_left=%u;cal=%d;inprog=%d;sps=%u",
             event, g_state.pills_dispensed_count, g_state.pills_missed_count,
             g_state.current_slot, g_state.pills_remaining, g_state.calibrated,
             g_state.motor_in_progress, g_state.steps_per_slot);
    if (lora_send_status(msg)) {
        printf("(LoRa) Status sent: %s\n", msg);
    } else {
        printf("(LoRa) Status send failed: %s\n", msg);
    }
}

// Recovery after power loss mid-turn: no rotation, resume from saved slot/state
static void safe_recover_if_mid_turn(void) {
    if (g_state.motor_in_progress) {
        printf("(RECOVERY) Power loss detected mid-turn. Resuming without rotation.\n");
        g_state.motor_in_progress = false;
        state_save();
        report_status("recovered_mid_turn");
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
    printf("(BOOT) Initializing...\n");

    leds_init();      printf("(INIT) LEDs initialized.\n");
    buttons_init();   printf("(INIT) Buttons initialized (CAL on GP14, START on GP15).\n");
    sensors_init();   printf("(INIT) Sensors initialized (Opto GP28, Piezo GP27).\n");
    stepper_init();   printf("(INIT) Stepper initialized (IN1 GP2, IN2 GP3, IN3 GP6, IN4 GP13).\n");
    eeprom_init();    printf("(INIT) EEPROM (AT24C256) initialized on I2C0 (SDA GP16, SCL GP17).\n");
    lora_init();      printf("(INIT) LoRa UART1 initialized (TX GP4, RX GP5).\n");

    state_load();
    g_state.boots_count++;
    state_save();
    printf("(STATE) Loaded persisted state: dispenses_done=%u, pills_left=%u, calibrated=%d, joined=%d, sps=%u\n",
           g_state.dispenses_done, g_state.pills_remaining, g_state.calibrated,
           g_state.joined_network, g_state.steps_per_slot);

    if (!g_state.joined_network) {
        printf("(LoRa) Attempting network join...\n");
        if (lora_join_network(LORA_APPKEY)) {
            g_state.joined_network = true;
            state_save();
            printf("(LoRa) Join successful. Network ready.\n");
            report_status("boot_join_success");
        } else {
            printf("(LoRa) Join failed. Will continue offline and retry later.\n");
            report_status("boot_join_failed");
        }
    } else {
        printf("(LoRa) Already joined. Reporting boot event.\n");
        report_status("boot");
    }

    // New recovery: resume from saved slot without rotating
    safe_recover_if_mid_turn();

    // Initialize system state based on saved calibration and progress
    system_state_t sys;
    if (g_state.calibrated) {
        if (g_state.dispenses_done < DISPENSE_SLOTS) {
            sys = SYS_DISPENSING;
            printf("(RECOVERY) Continuing dispensing from slot=%u\n", g_state.current_slot);
            report_status("resume_after_powerloss");
        } else {
            sys = SYS_READY_TO_START;
            printf("(RECOVERY) Cycle complete, ready to start new dispensing.\n");
        }
    } else {
        sys = SYS_WAIT_CAL_BUTTON;
        printf("(ACTION) Press button 1 to start wheel calibration.\n");
    }

    while (true) {
        switch (sys) {
            case SYS_WAIT_CAL_BUTTON: {
                leds_wait_blink();
                if (button_cal_pressed()) {
                    sys = SYS_CALIBRATING;
                    printf("(EVENT) Calibration button pressed. Starting calibration...\n");
                    report_status("calibration_start");
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

                    printf("(CAL) First revolution: %u steps\n", rev1);
                    printf("(CAL) Second revolution: %u steps\n", rev2);
                    printf("(CAL) Mean revolution: %u steps\n", mean);
                    printf("(CAL) Steps per slot: %u\n", g_state.steps_per_slot);
                    printf("(SUCCESS) Calibration complete. Press button 2 to start dispensing\n");
                    report_status("calibration_ok");
                    sys = SYS_READY_TO_START;
                } else {
                    leds_blink_error(5);
                    printf("(ERROR) Calibration failed. Try again.\n");
                    report_status("calibration_failed");
                    sys = SYS_WAIT_CAL_BUTTON;
                }
                break;
            }

            case SYS_READY_TO_START: {
                leds_on_ready();
                if (button_start_pressed()) {
                    printf("(EVENT) START button pressed. Beginning dispensing cycle.\n");
                    report_status("dispense_start");
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

                    printf("(MOTION) Advancing wheel to next slot (%u steps)...\n", g_state.steps_per_slot);
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

                    printf("(SENSOR) Checking piezo for pill hit...\n");
                    bool hit = piezo_detect_pill_hit(PIEZO_FALL_WINDOW_MS, PIEZO_DEBOUNCE_MS, PIEZO_MIN_EDGES);

                    g_state.dispenses_done++;

                    if (hit) {
                        g_state.pills_dispensed_count++;
                        if (g_state.pills_remaining > 0) g_state.pills_remaining--;
                        leds_dispense_progress(g_state.dispenses_done);
                        state_save();
                        printf("(SUCCESS) Pill detected.\n");
                        report_status("pill_dispensed");
                    } else {
                        g_state.pills_missed_count++;
                        leds_blink_error(5);
                        state_save();
                        printf("(WARNING) No pill detected.\n");
                        report_status("pill_not_detected");
                    }
                }

                printf("(INFO) One full circle complete. Dispenser empty.\n");
                report_status("dispenser_empty");
                g_state.calibrated = false;
                g_state.dispenses_done = 0;
                g_state.pills_remaining = DISPENSE_SLOTS;
                state_save();
                sys = SYS_EMPTY;
                break;
            }

            case SYS_EMPTY: {
                printf("(INFO) All pills dispensed. Dispenser is empty. Press button 1 to start aggain.\n");
                report_status("dispenser_empty");
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
