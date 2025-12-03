#pragma once
#include <stdint.h>
#include <stdbool.h>

bool eeprom_init(void);
bool eeprom_write(uint16_t addr, const uint8_t* data, uint16_t len);
bool eeprom_read(uint16_t addr, uint8_t* data, uint16_t len);
