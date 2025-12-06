#ifndef EEPROM_H
#define EEPROM_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void eeprom_init(void);
bool eeprom_read(uint16_t addr, uint8_t *buf, size_t len);
bool eeprom_write(uint16_t addr, const uint8_t *buf, size_t len);

#endif
