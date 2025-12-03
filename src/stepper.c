// src/stepper.c
#include "stepper.h"
#include "board.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "sensors.h"
#include "state.h"

// 4-phase sequence (adjust to your driver)
static const uint8_t seq[4][4] = {
    {1,0,1,0},
    {0,1,1,0},
    {0,1,0,1},
    {1,0,0,1}
};

static void drive_phase(int p){
    gpio_put(PIN_ST_IN1, seq[p][0]);
    gpio_put(PIN_ST_IN2, seq[p][1]);
    gpio_put(PIN_ST_IN3, seq[p][2]);
    gpio_put(PIN_ST_IN4, seq[p][3]);
}

void stepper_init(void){
    gpio_init(PIN_ST_IN1); gpio_set_dir(PIN_ST_IN1,true);
    gpio_init(PIN_ST_IN2); gpio_set_dir(PIN_ST_IN2,true);
    gpio_init(PIN_ST_IN3); gpio_set_dir(PIN_ST_IN3,true);
    gpio_init(PIN_ST_IN4); gpio_set_dir(PIN_ST_IN4,true);
    drive_phase(0);
}

// Mark motor active in EEPROM
void stepper_set_motor_active(bool active){
    DispenserState s;
    if (!state_load(&s)) return;
    s.motor_active = active ? 1 : 0;
    state_save(&s);
}

bool stepper_rotate_steps(int steps){
    if (steps <= 0) return true;
    stepper_set_motor_active(true);
    for (int i=0;i<steps;i++){
        drive_phase(i % 4);
        sleep_ms(3); // adjust speed to your mechanics
    }
    // stop (de-energize if desired)
    drive_phase(0);
    stepper_set_motor_active(false);
    return true;
}

bool stepper_calibrate_blocking(void){
    // Turn at least one full revolution and stop when opto reads 0
    int steps = 0;
    while (steps < CALIBRATION_FULL_STEPS || sensors_opto_read() != 0){
        drive_phase(steps % 4);
        steps++;
        sleep_ms(3);
        // Safety: if too long, break
        if (steps > CALIBRATION_FULL_STEPS * 4) break;
    }
    // Stop at detected opening alignment
    drive_phase(0);
    return sensors_opto_read() == 0;
}
