#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "config.h"
#include "sensors.h"
#include <stdlib.h>  // for abs()

void sensors_init(void) {
    // Opto sensor (digital input with pull-up)
    gpio_init(PIN_OPTO);
    gpio_set_dir(PIN_OPTO, GPIO_IN);
    gpio_pull_up(PIN_OPTO);

    // Piezo sensor on GP27 (ADC1)
    adc_init();
    adc_gpio_init(PIN_PIEZO);   // configure GPIO for ADC
    adc_select_input(1);        // GP27 = ADC1
}

bool opto_is_opening_at_sensor(void) {
    // Active LOW: returns true when hole is aligned
    return !gpio_get(PIN_OPTO);
}

bool piezo_detect_pill_hit(uint32_t window_ms, uint32_t debounce_ms, uint8_t min_edges) {
    uint32_t start = to_ms_since_boot(get_absolute_time());
    uint8_t edges = 0;
    uint16_t last = adc_read();

    while (to_ms_since_boot(get_absolute_time()) - start < window_ms) {
        uint16_t current = adc_read();
        if (abs((int)current - (int)last) > 300) {
            edges++;
            last = current;
            sleep_ms(debounce_ms);                   // debounce to avoid false triggers
        }
    }
    return edges >= min_edges;
}
