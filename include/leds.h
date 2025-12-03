#pragma once
#include <stdbool.h>
void leds_init(void);
void led_on(int idx, bool on);
void led_blink(int idx, int times, int on_ms, int off_ms);
