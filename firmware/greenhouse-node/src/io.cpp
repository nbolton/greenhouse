#include "io.h"

#include "attiny.h"
#include "pins.h"

#if IO_I2C_EN

#include <A85_PCF8574.h>
A85_PCF8574 io;

//#define PCF8574_ADDRESS 0x20 // PCF8574(N) - 0x20 to 0x27
#define PCF8574_ADDRESS 0x38  // PCF8574A(N) - 0x38 to 0x3F

#endif  // IO_I2C_EN

#if IO_I2C_EN

void io_init() { io.begin(PCF8574_ADDRESS); }

void shift() {}

void set(int pin) { io.setBit(pin, true); }

void clear(int pin) { io.setBit(pin, false); }

#elif IO_SR_EN

static int data = 0;

void io_init() {
  digitalWrite(PIN_SR_OE, LOW);

  pinMode(PIN_SR_OE, OUTPUT);
  pinMode(PIN_SR_LATCH, OUTPUT);
  pinMode(PIN_SR_CLOCK, OUTPUT);
  pinMode(PIN_SR_DATA, OUTPUT);
}

void shift() {
  digitalWrite(PIN_SR_LATCH, LOW);
  shiftOut(PIN_SR_DATA, PIN_SR_CLOCK, MSBFIRST, data);
  digitalWrite(PIN_SR_LATCH, HIGH);
}

void set(int pin) { bitSet(data, pin); }

void clear(int pin) { bitClear(data, pin); }

#else

void shift() {}

void set(int pin) {}

void clear(int pin) {}

#endif
