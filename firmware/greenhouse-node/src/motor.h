#pragma once

#include "attiny.h"

void motor_init();
void motor_loop();
bool motor_run(byte dir, byte secs);
