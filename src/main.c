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
        printf("[LoRa] Status sent: %s\n", msg);
    } else {
        printf("[LoRa] Status send failed: %s\n", msg);
    }
}

static void safe_recover_if_mid_turn(void) {
    if (g_state.motor_in_progress) {
        printf("[RECOVERY] Detected power-off during motion. Starting safe auto-recalibration...\n");
        // Scan up to nominal two revolutions to find opening
        for (uint32_t i = 0; i < NOMINAL_FULL_REV_STEPS * 2; ++i) {
            stepper_step_sequence_once();
            sleep_us(STEPPER_STEP_DELAY_US);
            if (opto_is_opening_at_sensor()) {
                printf("[RECOVERY] Optical opening detected. Aligning to calibration slot.\n");
                g_state.current_slot = CALIBRATION_SLOT_INDEX;
                g_state.calibrated = true;
                stepper_mark_motion_end();
                g_state.motor_in_progress = false;
                state_save();
                report_status("recalibrated_after_powerloss");
                printf("[RECOVERY] Recalibration done. Ready for user calibration if needed.\n");
                return;
            }
        }
        // Not found -  mark that recalibration required
        stepper_mark_motion_end();
        g_state.motor_in_progress = false;
        g_state.calibrated = false;
        state_save();
        report_status("recalibration_required");
        printf("[RECOVERY] Opening not found. Please press CALIBRATION button to recalibrate.\n");
    }
}

int main() {
    stdio_init_all();              // init USB and UART stdio
    sleep_ms(2000);                // small wait to open terminal
    setvbuf(stdout, NULL, _IONBF, 0); // disable buffering

    printf("=== Pill Dispenser Firmware Started ===\n");
    printf("[BOOT] Initializing subsystems...\n");

    leds_init();      printf("[INIT] LEDs initialized.\n");
    buttons_init();   printf("[INIT] Buttons initialized (CAL on GP14, START on GP15).\n");
    sensors_init();   printf("[INIT] Sensors initialized (Opto GP28, Piezo GP27).\n");
    stepper_init();   printf("[INIT] Stepper initialized (IN1 GP2, IN2 GP3, IN3 GP6, IN4 GP13).\n");
    eeprom_init();    printf("[INIT] EEPROM (AT24C256) initialized on I2C0 (SDA GP16, SCL GP17).\n");
    lora_init();      printf("[INIT] LoRa UART1 initialized (TX GP4, RX GP5).\n");

    // load persisted state
    state_load();
    g_state.boots_count++;
    state_save();
    printf("[STATE] Loaded persisted state: dispenses_done=%u, pills_left=%u, calibrated=%d, joined=%d, sps=%u\n",
           g_state.dispenses_done, g_state.pills_remaining, g_state.calibrated, g_state.joined_network, g_state.steps_per_slot);

    // join LoRa
    if (!g_state.joined_network) {
        printf("[LoRa] Attempting network join...\n");
        if (lora_join_network(LORA_APPKEY)) {
            g_state.joined_network = true;
            state_save();
            printf("[LoRa] Join successful. Network ready.\n");
            report_status("boot_join_success");
        } else {
            printf("[LoRa] Join failed. Will continue offline and retry later.\n");
            report_status("boot_join_failed");
        }
    } else {
        printf("[LoRa] Already joined. Reporting boot event.\n");
        report_status("boot");
    }

    // recover if power-off occurred mid-turn
    safe_recover_if_mid_turn();

    system_state_t sys = SYS_WAIT_CAL_BUTTON;
    printf("[ACTION] Press CALIBRATION button to start wheel calibration.\n");

    while (true) {
        switch (sys) {
            case SYS_WAIT_CAL_BUTTON: {
                leds_wait_blink();
                if (button_cal_pressed()) {
                    sys = SYS_CALIBRATING;
                    printf("[EVENT] Calibration button pressed. Starting calibration...\n");
                    report_status("calibration_start");
                }
                break;
            }

            case SYS_CALIBRATING: {
                //calibration - making two turns, find mean from total steps, divide by 8 to find steps per slot
                stepper_mark_motion_begin();

                printf("[CAL] Starting first revolution count...\n");
                uint32_t rev1 = stepper_calibrate_revolution();
                sleep_ms(300);
                printf("[CAL] Starting second revolution count...\n");
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

                    printf("[CAL] First revolution: %u steps\n", rev1);
                    printf("[CAL] Second revolution: %u steps\n", rev2);
                    printf("[CAL] Mean revolution: %u steps\n", mean);
                    printf("[CAL] Steps per slot: %u\n", g_state.steps_per_slot);
                    printf("[SUCCESS] Calibration complete. Ready to dispense.\n");
                    report_status("calibration_ok");
                    sys = SYS_READY_TO_START;
                } else {
                    leds_blink_error(5);
                    printf("[ERROR] Calibration failed. Try again.\n");
                    report_status("calibration_failed");
                    sys = SYS_WAIT_CAL_BUTTON;
                }
                break;
            }

            case SYS_READY_TO_START: {
                leds_on_ready();
                if (button_start_pressed()) {
                    printf("[EVENT] START button pressed. Beginning dispensing cycle.\n");
                    report_status("dispense_start");
                    sys = SYS_DISPENSING;
                }
                break;
            }

            case SYS_DISPENSING: {
                if (g_state.dispenses_done >= DISPENSE_SLOTS) {
                    sys = SYS_EMPTY;
                    break;
                }

                printf("[INFO] Waiting %u seconds before next dispensing turn.\n",
                       DISPENSE_INTERVAL_MS / 1000);
                uint32_t t0 = now_ms();
                while (now_ms() - t0 < DISPENSE_INTERVAL_MS) {
                    tight_loop_contents();
                }

                printf("[MOTION] Advancing wheel to next slot (%u steps)...\n", g_state.steps_per_slot);
                stepper_mark_motion_begin();
                stepper_advance_one_slot();
                stepper_mark_motion_end();
                slot_advance();
                printf("[MOTION] Wheel advanced. Current slot=%u\n", g_state.current_slot);

                printf("[SENSOR] Checking piezo sensor for pill hit within %u ms window...\n",
                       PIEZO_FALL_WINDOW_MS);
                bool hit = piezo_detect_pill_hit(PIEZO_FALL_WINDOW_MS, PIEZO_DEBOUNCE_MS, PIEZO_MIN_EDGES);

                if (hit) {
                    g_state.pills_dispensed_count++;
                    g_state.dispenses_done++;
                    if (g_state.pills_remaining > 0) g_state.pills_remaining--;
                    leds_dispense_progress(g_state.dispenses_done);
                    state_save();
                    printf("[SUCCESS] Pill detected. Dispenses_done=%u, Pills_left=%u\n",
                           g_state.dispenses_done, g_state.pills_remaining);
                    report_status("pill_dispensed");
                } else {
                    g_state.pills_missed_count++;
                    leds_blink_error(5);
                    state_save();
                    printf("[WARNING] No pill detected. LED blinked 5 times. Miss_count=%u\n",
                           g_state.pills_missed_count);
                    report_status("pill_not_detected");
                }

                if (g_state.dispenses_done >= DISPENSE_SLOTS) {
                    sys = SYS_EMPTY;
                }

                break;
            }

            case SYS_EMPTY: {
                printf("[INFO] All 7 pills dispensed. Dispenser is empty.\n");
                printf("[ACTION] Refill pills and press CALIBRATION button to restart.\n");
                report_status("dispenser_empty");

                g_state.calibrated = false; // force calibration again
                state_save();
                sys = SYS_WAIT_CAL_BUTTON;
                break;
            }

            default:
                sys = SYS_WAIT_CAL_BUTTON;
                break;
        }
        sleep_ms(10);
    }
    return 0;
}
