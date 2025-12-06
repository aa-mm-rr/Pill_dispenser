#ifndef SENSORS_H
#define SENSORS_H
#include <stdbool.h>

void sensors_init(void);
bool opto_is_opening_at_sensor(void);
bool piezo_detect_pill_hit(uint32_t window_ms, uint32_t debounce_ms, uint8_t min_edges);

#endif
