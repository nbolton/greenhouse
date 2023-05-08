#pragma once

#include <PCF8574.h> // https://github.com/xreef/PCF8574_library
#include <Wire.h>

// IO 0 (U1)
#define IO_DEV 0
#define FAN_EN P0
#define BEEP_EN P1
#define PSU_LED P2
#define BATT_LED P3
#define BATT_EN P4
#define PSU_EN P5
#define AC_EN P6

#define IO_DEVICES 2
#define IO_PORTS 8

namespace io {

bool init();
bool begin();
bool begin(PCF8574 &pcf8574, int device);
void blink();
void blink(PCF8574 &pcf8574, int device);
PCF8574 &device(int device);
void toggle(int device, int port);
void beep(int times);
void set(int device, int port, bool value);

} // namespace io
