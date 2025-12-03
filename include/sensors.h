#pragma once
#include <stdbool.h>
#include <stdint.h>

void sensors_init(void);
int  sensors_opto_read(void);
bool sensors_piezo_falling_seen(uint32_t window_ms);
