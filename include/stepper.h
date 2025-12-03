// include/stepper.h
#pragma once
#include <stdbool.h>
void stepper_init(void);
void stepper_set_motor_active(bool active); // state flag for power-off detection
bool stepper_rotate_steps(int steps);
bool stepper_calibrate_blocking(void); // turn >1 full rev, stop when opto == 0
