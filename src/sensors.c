#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "config.h"
#include "sensors.h"
#include "util.h"

void sensors_init(void) {
    gpio_init(PIN_OPTO);
    gpio_set_dir(PIN_OPTO, GPIO_IN);
    gpio_pull_up(PIN_OPTO);

    gpio_init(PIN_PIEZO);
    gpio_set_dir(PIN_PIEZO, GPIO_IN);
    gpio_pull_up(PIN_PIEZO);
}

bool opto_is_opening_at_sensor(void) {
    // Reads zero when opening is at sensor
    return gpio_get(PIN_OPTO) == 0;
}

bool piezo_detect_pill_hit(uint32_t window_ms, uint32_t debounce_ms, uint8_t min_edges) {
    uint32_t start = now_ms();
    uint8_t edges = 0;
    bool last = gpio_get(PIN_PIEZO);
    uint32_t last_edge_time = start;

    while (now_ms() - start < window_ms) {
        bool val = gpio_get(PIN_PIEZO);
        if (last && !val) {
            // falling edge
            uint32_t t = now_ms();
            if (t - last_edge_time >= debounce_ms) {
                edges++;
                last_edge_time = t;
            }
        }
        last = val;
        sleep_ms(1);
    }
    return edges >= min_edges;
}
