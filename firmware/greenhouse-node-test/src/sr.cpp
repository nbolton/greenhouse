#include "sr.h"

#include <Arduino.h>

#include "pins.h"

static int data = 0;

void sr_init() {
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

void set_shift(int pin) {
  set(pin);
  shift();
}

void clear_shift(int pin) {
  clear(pin);
  shift();
}
