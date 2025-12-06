#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "config.h"
#include "leds.h"

void leds_init(void) {
    gpio_init(PIN_LED1); gpio_set_dir(PIN_LED1, GPIO_OUT);
    gpio_init(PIN_LED2); gpio_set_dir(PIN_LED2, GPIO_OUT);
    gpio_init(PIN_LED3); gpio_set_dir(PIN_LED3, GPIO_OUT);
    leds_all_off();
}

void leds_all_off(void) {
    gpio_put(PIN_LED1, 0);
    gpio_put(PIN_LED2, 0);
    gpio_put(PIN_LED3, 0);
}

void leds_wait_blink(void) {
    static uint32_t t = 0;
    static bool on = false;
    if (time_us_32() - t > 500000) { // 0.5s
        on = !on;
        gpio_put(PIN_LED1, on);
        gpio_put(PIN_LED2, 0);
        gpio_put(PIN_LED3, 0);
        t = time_us_32();
    }
}

void leds_on_ready(void) {
    gpio_put(PIN_LED1, 0);
    gpio_put(PIN_LED2, 1);
    gpio_put(PIN_LED3, 0);
}

void leds_blink_error(uint8_t times) {
    for (uint8_t i = 0; i < times; ++i) {
        gpio_put(PIN_LED3, 1);
        sleep_ms(150);
        gpio_put(PIN_LED3, 0);
        sleep_ms(150);
    }
}

void leds_dispense_progress(uint8_t count) {
    // Simple progress indicator: light LEDs based on count
    gpio_put(PIN_LED1, count >= 1);
    gpio_put(PIN_LED2, count >= 3);
    gpio_put(PIN_LED3, count >= 5);
}
