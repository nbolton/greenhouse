#pragma once

#include "attiny.h"

void motor_init();
void motor_loop();
void motor_speed(byte pwm);
bool motor_run(byte dir, byte secs);
