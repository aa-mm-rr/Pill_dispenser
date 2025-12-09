#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "config.h"
#include "sensors.h"

// Global flag set by interrupt
static volatile bool piezo_triggered = false;

// Interrupt handler: must return void
void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == PIN_PIEZO && (events & (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE))) {
        piezo_triggered = true;   // mark that a pill hit was detected
    }
}

void sensors_init(void) {

    gpio_init(PIN_OPTO);
    gpio_set_dir(PIN_OPTO, GPIO_IN);
    gpio_pull_up(PIN_OPTO);

    gpio_init(PIN_PIEZO);
    gpio_set_dir(PIN_PIEZO, GPIO_IN);
    gpio_pull_up(PIN_PIEZO);

    // Register interrupt for piezo
    gpio_set_irq_enabled_with_callback(
        PIN_PIEZO,
        GPIO_IRQ_EDGE_FALL,
        true,
        &gpio_irq_handler
    );
}

// This must exist for stepper.c to link!
bool opto_is_opening_at_sensor(void) {
    return !gpio_get(PIN_OPTO);
}

bool piezo_was_triggered(void) {
    return piezo_triggered;
}

void piezo_reset_flag(void) {
    piezo_triggered = false;
}
