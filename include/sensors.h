#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>
#include <stdint.h>

// Initialize both opto and piezo sensors
void sensors_init(void);

// Opto sensor: returns true if opening is aligned with sensor
bool opto_is_opening_at_sensor(void);

// Piezo sensor: detect pill hit by monitoring ADC fluctuations
bool piezo_detect_pill_hit(uint32_t window_ms, uint32_t debounce_ms, uint8_t min_edges);

#endif
