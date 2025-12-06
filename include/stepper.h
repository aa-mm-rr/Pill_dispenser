#ifndef STEPPER_H
#define STEPPER_H
#include <stdbool.h>
#include <stdint.h>

void stepper_init(void);
void stepper_mark_motion_begin(void);
void stepper_mark_motion_end(void);

void stepper_step_sequence_once(void);
void stepper_steps(uint32_t steps);

void stepper_advance_one_slot(void);
void stepper_full_turn_nominal(void);

uint32_t stepper_calibrate_revolution(void);

void slot_set(uint8_t slot_index);
void slot_advance(void);
uint8_t slot_get(void);

#endif
