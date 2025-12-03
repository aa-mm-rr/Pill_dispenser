#include "pico/stdlib.h"
#include "board.h"
#include "buttons.h"
#include "leds.h"
#include "sensors.h"
#include "stepper.h"
#include "state.h"
#include "eeprom_at24c256.h"
#include "lora.h"
#include "util.h"
#include <stdio.h>

static void wait_for_button_and_blink(void) {
    printf("Press button 1 to start calibration...\n");
    while (!button_read(1)) {
        led_on(0, true);
        sleep_ms(300);
        led_on(0, false);
        sleep_ms(300);
    }
}

static void indicate_error(void) {
    printf("⚠️ Pill not detected! Blinking LED2...\n");
    led_blink(2, 5, 150, 150);
}

int main() {
    stdio_init_all();
    leds_init();
    buttons_init();
    sensors_init();
    stepper_init();
    eeprom_init();

    #include "pico/stdlib.h"
#include "board.h"
#include "buttons.h"
#include "leds.h"
#include "sensors.h"
#include "stepper.h"
#include "state.h"
#include "eeprom_at24c256.h"
#include "lora.h"
#include "util.h"
#include <stdio.h>

static void wait_for_button_and_blink(void) {
    printf("Press button 1 to start calibration...\n");
    while (!button_read(1)) {
        led_on(0, true);
        sleep_ms(300);
        led_on(0, false);
        sleep_ms(300);
    }
}

static void indicate_error(void) {
    printf("⚠️ Pill not detected! Blinking LED2...\n");
    led_blink(2, 5, 150, 150);
}

int main() {
    stdio_init_all();
    leds_init();
    buttons_init();
    sensors_init();
    stepper_init();
    eeprom_init();

    DispenserState st;
    if (!state_load(&st)) {
        state_reset(&st);
        state_load(&st);
    }
    st.boot_count++;
    state_save(&st);

    printf("Pill dispenser starting up...\n");
    printf("System boot count: %u\n", st.boot_count);

    // LoRa init
    lora_init();
    const char* APPKEY = "2B7E151628AED2A6ABF7158809CF4F3C";
    if (lora_join_otaa(APPKEY)) {
        printf("LoRaWAN joined successfully.\n");
        lora_report("boot");
    } else {
        printf("LoRaWAN join failed.\n");
    }

    while (true) {
        // 1) Wait for calibration
        wait_for_button_and_blink();

        printf("Calibrating dispenser wheel...\n");
        bool calib_ok = stepper_calibrate_blocking();
        if (calib_ok) {
            printf("✅ Calibration successful.\n");
            st.calibrated = 1;
        } else {
            printf("❌ Calibration failed. Please try again.\n");
            st.calibrated = 0;
        }
        st.pills_dispensed = 0;
        state_save(&st);

        // Keep LED0 on until user presses BTN2
        led_on(0, true);
        printf("Calibration done. Press button 2 to start dispensing.\n");
        while (!button_read(2)) sleep_ms(50);

        printf("Starting pill dispensing loop...\n");
        periodic_t ticker;
        periodic_start(&ticker, millis(), DISPENSE_INTERVAL_MS);

        while (st.pills_dispensed < TOTAL_PILLS) {
            while (!periodic_due(&ticker, millis())) sleep_ms(10);

            printf("Dispensing pill %d...\n", st.pills_dispensed + 1);
            const int STEPS_PER_SLOT = CALIBRATION_FULL_STEPS / 8;
            stepper_rotate_steps(STEPS_PER_SLOT);

            printf("Checking piezo sensor for pill drop...\n");
            bool pill_ok = sensors_piezo_falling_seen(PIEZO_FALL_WINDOW_MS);
            if (pill_ok) {
                st.pills_dispensed++;
                state_save(&st);
                printf("✅ Pill %d detected successfully.\n", st.pills_dispensed);
                led_on(1, true); sleep_ms(150); led_on(1, false);
                lora_report("pill_ok");
            } else {
                indicate_error();
                lora_report("pill_miss");
            }
        }

        printf("All pills dispensed. Cycle complete.\n");
        lora_report("empty");

        // Reset state
        st.calibrated = 0;
        st.pills_dispensed = 0;
        state_save(&st);
        led_on(0, false);
        printf("System reset. Press button 1 to start calibration again.\n");
    }

    return 0;
}


    DispenserState st;
    if (!state_load(&st)) {
        state_reset(&st);
        state_load(&st);
    }
    st.boot_count++;
    state_save(&st);

    printf("Pill dispenser starting up...\n");
    printf("System boot count: %u\n", st.boot_count);

    // LoRa init
    lora_init();
    const char* APPKEY = "2B7E151628AED2A6ABF7158809CF4F3C";
    if (lora_join_otaa(APPKEY)) {
        printf("LoRaWAN joined successfully.\n");
        lora_report("boot");
    } else {
        printf("LoRaWAN join failed.\n");
    }

    while (true) {
        // 1) Wait for calibration
        wait_for_button_and_blink();

        printf("Calibrating dispenser wheel...\n");
        bool calib_ok = stepper_calibrate_blocking();
        if (calib_ok) {
            printf("Calibration successful.\n");
            st.calibrated = 1;
        } else {
            printf("Calibration failed. Please try again.\n");
            st.calibrated = 0;
        }
        st.pills_dispensed = 0;
        state_save(&st);

        // Keep LED0 on until user presses BTN2
        led_on(0, true);
        printf("Calibration done. Press button 2 to start dispensing.\n");
        while (!button_read(2)) sleep_ms(50);

        printf("Starting pill dispensing loop...\n");
        periodic_t ticker;
        periodic_start(&ticker, millis(), DISPENSE_INTERVAL_MS);

        while (st.pills_dispensed < TOTAL_PILLS) {
            while (!periodic_due(&ticker, millis())) sleep_ms(10);

            printf("Dispensing pill %d...\n", st.pills_dispensed + 1);
            const int STEPS_PER_SLOT = CALIBRATION_FULL_STEPS / 8;
            stepper_rotate_steps(STEPS_PER_SLOT);

            printf("Checking piezo sensor for pill drop...\n");
            bool pill_ok = sensors_piezo_falling_seen(PIEZO_FALL_WINDOW_MS);
            if (pill_ok) {
                st.pills_dispensed++;
                state_save(&st);
                printf("✅ Pill %d detected successfully.\n", st.pills_dispensed);
                led_on(1, true); sleep_ms(150); led_on(1, false);
                lora_report("pill_ok");
            } else {
                indicate_error();
                lora_report("pill_miss");
            }
        }

        printf("All pills dispensed. Cycle complete.\n");
        lora_report("empty");

        // Reset state
        st.calibrated = 0;
        st.pills_dispensed = 0;
        state_save(&st);
        led_on(0, false);
        printf("System reset. Press button 1 to start calibration again.\n");
    }

    return 0;
}
