#include "sr.h"

#include "attiny.h"

#if SR_EN

static int data = 0;

void sr_init() {
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
