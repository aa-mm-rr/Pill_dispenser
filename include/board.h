#pragma once
#include "pico/stdlib.h"

// Sensors
#define PIN_OPTO       28  // reads 0 when opening is at sensor
#define PIN_PIEZO      27  // falling edges when pill hits

// Stepper driver IN1–IN4 (outputs)
#define PIN_ST_IN1      13
#define PIN_ST_IN2      6
#define PIN_ST_IN3      3
#define PIN_ST_IN4     2

// LoRaWAN UART1
#define LORA_UART       uart1
#define PIN_LORA_TX       4 // GP4
#define PIN_LORA_RX       5 // GP5
#define LORA_BAUD     9600  // default

// LEDs
#define PIN_LED0       22   // GP20
#define PIN_LED1       23   // GP21
#define PIN_LED2       24   // GP22

// Buttons
#define PIN_BTN1       9   // GP14
#define PIN_BTN2       8   // GP15
#define PIN_BTN3       7  // GP16

// EEPROM
#define EEPROM_I2C     i2c0
#define PIN_I2C_SDA    16   // GP19
#define PIN_I2C_SCL    17   // GP18
#define EEPROM_ADDR    0x50 // A0/A1 = GND

// App parameters
#define DISPENSE_INTERVAL_MS 30000 // 30s between dispensing for test
#define TOTAL_PILLS          7     // wheel turns 7 times for pills (1 slot is calibration)
#define PIEZO_FALL_WINDOW_MS 800   // window to detect falling edge considering ~3.5cm drop
#define CALIBRATION_FULL_STEPS 512 // steps to ensure >1 full turn (adjust to your motor/gear)
