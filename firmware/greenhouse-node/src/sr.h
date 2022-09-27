#pragma once

#include "pins.h"

#define SR_TOTAL 1

#define SR_PIN_LED_ON 0
#define SR_PIN_LED_ERR 1
#define SR_PIN_LED_RX 2
#define SR_PIN_LED_TX 3
#define SR_PIN_MOTOR_B 4
#define SR_PIN_MOTOR_A 5

void sr_init();
void shift();
void set(int pin);
void clear(int pin);
