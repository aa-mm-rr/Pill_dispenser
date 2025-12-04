#include "sensors.h"
#include "board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

void sensors_init(void){
    gpio_init(PIN_OPTO);
    gpio_set_dir(PIN_OPTO, false);
    gpio_pull_up(PIN_OPTO);

    gpio_init(PIN_PIEZO);
    gpio_set_dir(PIN_PIEZO, false);
    gpio_pull_up(PIN_PIEZO);
}

int sensors_opto_read(void){
    return gpio_get(PIN_OPTO); // 0 when opening is at sensor
}

bool sensors_piezo_falling_seen(uint32_t window_ms){
    absolute_time_t deadline = make_timeout_time_ms(window_ms);

    while (!time_reached(deadline)) {
        // Faster sampling: ~10 kHz
        sleep_us(100);

        int now = gpio_get(PIN_PIEZO);

        // Trigger on ANY single low sample
        if (now == 0) {
            return true;
        }
    }
    return false;
}

