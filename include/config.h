#ifndef CONFIG_H
#define CONFIG_H

// GPIO
#define PIN_OPTO          28
#define PIN_PIEZO         27

#define PIN_STEPPER_IN1    2
#define PIN_STEPPER_IN2    3
#define PIN_STEPPER_IN3    6
#define PIN_STEPPER_IN4   13

#define PIN_LED1          20
#define PIN_LED2          21
#define PIN_LED3          22

#define PIN_BTN_CAL       9
#define PIN_BTN_START     8

// LoRa
#define LORA_UART_ID      uart1
#define PIN_LORA_TX       4
#define PIN_LORA_RX       5
#define LORA_BAUD         9600

// I2C EEPROM
#define I2C_ID            i2c0
#define PIN_I2C_SDA       16
#define PIN_I2C_SCL       17
#define I2C_BAUD          400000
#define EEPROM_ADDR       0x50

// Dispenser configuration
#define TOTAL_COMPARTMENTS        8
#define DISPENSE_SLOTS            7
#define CALIBRATION_SLOT_INDEX    0  // slot with optical opening aligned with sensor

// Timing (testing mode)
#define DISPENSE_INTERVAL_MS      5000  // 5s for testing, change to 30 seconds later
#define STEPPER_STEP_DELAY_US     2000   // ~833 Hz

#define NOMINAL_FULL_REV_STEPS    4096
#define NOMINAL_SLOT_STEPS        (NOMINAL_FULL_REV_STEPS / TOTAL_COMPARTMENTS)


#define PIEZO_FALL_WINDOW_MS      1000   // window to detect drop. needed at least 270
#define PIEZO_DEBOUNCE_MS         0     // debounce successive edges
#define PIEZO_MIN_EDGES           1      // at least one falling edge counts as "pill hit"

/*
 * LoRa settings
#define LORA_PORT                 8
#define LORA_CLASS                'A'
#define LORA_MODE                 "LWOTAA"

 Retry or timeout
#define LORA_JOIN_TIMEOUT_MS      20000  // join can take up to ~20s
#define LORA_CMD_RESP_TIMEOUT_MS  5000
#define LORA_MSG_TIMEOUT_MS       10000
*/

// EEPROM layout
#define STATE_MAGIC       0xA1B2C3D4
#define STATE_VERSION     1
#define EEPROM_STATE_ADDR 0x0000     // start of EEPROM
#define EEPROM_STATE_SIZE 128        // enough for struct

#endif
