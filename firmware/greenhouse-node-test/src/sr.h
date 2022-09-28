#pragma once

#define SR_TOTAL 1

#define SR_PIN_LED_ERR 1
#define SR_PIN_LED_RX 2
#define SR_PIN_LED_TX 3

void sr_init();
void shift();
void set(int pin);
void clear(int pin);
void set_shift(int pin);
void clear_shift(int pin);
