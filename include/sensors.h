#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>
#include <stdint.h>

// Initialize sensors
void sensors_init(void);

// Opto sensor check
bool opto_is_opening_at_sensor(void);

// Piezo interrupt helpers
bool piezo_was_triggered(void);
void piezo_reset_flag(void);

#endif
