#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "config.h"
#include "buttons.h"

static bool debounced_read(uint pin) {
    bool a = gpio_get(pin);
    sleep_ms(5);
    bool b = gpio_get(pin);
    return a && b;
}

void buttons_init(void) {
    gpio_init(PIN_BTN_CAL);
    gpio_set_dir(PIN_BTN_CAL, GPIO_IN);
    gpio_pull_up(PIN_BTN_CAL);

    gpio_init(PIN_BTN_START);
    gpio_set_dir(PIN_BTN_START, GPIO_IN);
    gpio_pull_up(PIN_BTN_START);
}

bool button_cal_pressed(void) {
    // active low with pull-up assumption
    return !debounced_read(PIN_BTN_CAL);
}

bool button_start_pressed(void) {
    return !debounced_read(PIN_BTN_START);
}
