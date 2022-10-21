#pragma once

#if IO_SR_EN || IO_I2C_EN
#define IO_EN 1
#else
#define IO_EN 0
#endif

#if IO_SR_EN

#define SR_PIN_LED_ON 0
#define SR_PIN_LED_ERR 1
#define SR_PIN_LED_RX 2
#define SR_PIN_LED_TX 3
#define SR_PIN_MOTOR_B 4
#define SR_PIN_MOTOR_A 5

// LED current sourced
#define LED_ON set
#define LED_OFF clear

#elif IO_I2C_EN

#define SR_PIN_LED_ON 4
#define SR_PIN_LED_ERR 5
#define SR_PIN_LED_RX 6
#define SR_PIN_LED_TX 7
#define SR_PIN_MOTOR_B 3
#define SR_PIN_MOTOR_A 2

// LED current sinking
#define LED_ON clear
#define LED_OFF set

#endif

void io_init();
void shift();
void set(int pin);
void clear(int pin);
