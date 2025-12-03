#include "eeprom_at24c256.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "board.h"
#include <string.h>

static bool ack_poll(void) {
    // Poll until device ACKs (write cycle complete)
    for (int i = 0; i < 50; i++) {
        uint8_t dummy = 0;
        int ret = i2c_write_blocking(EEPROM_I2C, EEPROM_ADDR, &dummy, 0, false);
        if (ret >= 0) return true;
        sleep_ms(1);
    }
    return false;
}

bool eeprom_init(void) {
    i2c_init(EEPROM_I2C, 400 * 1000);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);
    sleep_ms(10);
    return true;
}

bool eeprom_write(uint16_t addr, const uint8_t* data, uint16_t len) {
    uint16_t pos = 0;
    while (pos < len) {
        uint16_t page_off = addr % 64;
        uint16_t chunk = 64 - page_off;
        if (chunk > (len - pos)) chunk = (len - pos);

        uint8_t buf[2 + 64];
        buf[0] = (uint8_t)(addr >> 8);
        buf[1] = (uint8_t)(addr & 0xFF);
        memcpy(buf + 2, data + pos, chunk);

        if (i2c_write_blocking(EEPROM_I2C, EEPROM_ADDR, buf, 2 + chunk, false) < 0) return false;
        if (!ack_poll()) return false;

        addr += chunk;
        pos += chunk;
    }
    return true;
}

bool eeprom_read(uint16_t addr, uint8_t* data, uint16_t len) {
    uint8_t ab[2] = {(uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF)};
    if (i2c_write_blocking(EEPROM_I2C, EEPROM_ADDR, ab, 2, true) < 0) return false;
    if (i2c_read_blocking(EEPROM_I2C, EEPROM_ADDR, data, len, false) < 0) return false;
    return true;
}
