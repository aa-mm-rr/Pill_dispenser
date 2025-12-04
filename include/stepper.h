// include/stepper.h
#pragma once
#include <stdbool.h>
#include <stdint.h>

void stepper_init(void);
void stepper_set_motor_active(bool active); // state flag for power-off detection
bool stepper_rotate_steps(int steps);
bool stepper_calibrate_blocking(void); // turn >1 full rev, stop when opto == 0
bool stepper_rotate_and_check(int steps, uint32_t window_ms);
