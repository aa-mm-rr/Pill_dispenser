#ifndef LEDS_H
#define LEDS_H
#include <stdint.h>

void leds_init(void);
void leds_all_off(void);
void leds_wait_blink(void);
void leds_on_ready(void);
void leds_blink_error(uint8_t times);
void leds_dispense_progress(uint8_t count);

#endif
