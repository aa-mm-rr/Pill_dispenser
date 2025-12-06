#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "eeprom.h"
#include "config.h"

void eeprom_init(void) {
    i2c_init(I2C_ID, I2C_BAUD);
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);
}

bool eeprom_read(uint16_t addr, uint8_t *buf, size_t len) {
    uint8_t addr_buf[2] = { (uint8_t)(addr >> 8), (uint8_t)(addr & 0xFF) };
    int w = i2c_write_blocking(I2C_ID, EEPROM_ADDR, addr_buf, 2, true);
    if (w < 0) return false;
    int r = i2c_read_blocking(I2C_ID, EEPROM_ADDR, buf, len, false);
    return r >= 0;
}

bool eeprom_write(uint16_t addr, const uint8_t *buf, size_t len) {
    size_t written = 0;
    while (written < len) {
        uint16_t a = addr + written;
        size_t chunk = len - written;
        if (chunk > 16) chunk = 16;

        uint8_t tmp[18];
        tmp[0] = (uint8_t)(a >> 8);
        tmp[1] = (uint8_t)(a & 0xFF);
        for (size_t i = 0; i < chunk; ++i) tmp[2 + i] = buf[written + i];

        int w = i2c_write_blocking(I2C_ID, EEPROM_ADDR, tmp, 2 + chunk, false);
        if (w < 0) return false;

        sleep_ms(5); // EEPROM write cycle time
        written += chunk;
    }
    return true;
}
