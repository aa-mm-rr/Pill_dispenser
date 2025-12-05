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
    led_blink(2, 5, 200, 200);
}

int main() {
    stdio_init_all();
    leds_init();
    buttons_init();
    sensors_init();
    stepper_init();
    eeprom_init();
    sleep_ms(2000); // small wait to open terminal

    DispenserState st;
    if (!state_load(&st)) {
        state_reset(&st);
        state_load(&st);
    }
    st.boot_count++;
    state_save(&st);

    printf("Pill dispenser starting up...\n");
    printf("System boot count: %u\n", st.boot_count);
    printf("Stored steps_per_rev: %u\n", st.steps_per_rev);

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

        bool calib_ok = stepper_calibrate_blocking();
        // Reload state to pick up stored steps_per_rev from calibration
        state_load(&st);

        st.calibrated = calib_ok ? 1 : 0;
        st.pills_dispensed = 0;
        state_save(&st);

        // Keep LED0 on until user presses BTN2
        led_on(0, true);
        printf("Calibration done. Measured steps_per_rev: %u\n", st.steps_per_rev);
        printf("Press button 2 to start dispensing.\n");
        while (!button_read(2)) sleep_ms(50);

        printf("Starting pill dispensing...\n");
        periodic_t ticker;
        periodic_start(&ticker, millis(), DISPENSE_INTERVAL_MS);

        // 8 slots - 7 for pills
        const int total_pills = TOTAL_PILLS; // should be 7
        const int steps_per_slot = st.steps_per_rev / 8;

        printf("Using steps_per_rev=%d, steps_per_slot=%d\n", st.steps_per_rev, steps_per_slot);

        while (st.pills_dispensed < total_pills) {
            while (!periodic_due(&ticker, millis())) sleep_ms(10);

            int next_pill_num = st.pills_dispensed + 1;
            printf("Dispensing pill %d of %d...\n", next_pill_num, total_pills);

            printf("Checking piezo sensor for pill drop...\n");
            bool pill_ok = stepper_rotate_and_check(steps_per_slot, PIEZO_FALL_WINDOW_MS);


            if (pill_ok) {
                printf("Pill %d detected successfully.\n", next_pill_num);
                led_on(1, true); sleep_ms(150); led_on(1, false);
                lora_report("pill_ok");
            } else {
                printf("Pill %d not detected.\n", next_pill_num);
                indicate_error();
                lora_report("pill_miss");
            }

            // go to next pill
            st.pills_dispensed++;
            state_save(&st);
        }

        printf("All pills dispensed. Cycle complete.\n");
        lora_report("empty");

        // reset state for next cycle
        st.calibrated = 0;
        st.pills_dispensed = 0;
        state_save(&st);
        led_on(0, false);
        printf("System reset. Press button 1 to start calibration again.\n");
    }

    return 0;
}
