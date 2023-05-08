#pragma once

#include <Adafruit_INA219.h>

namespace power {

bool init();
bool init(Adafruit_INA219 &ina219, int i);
bool update();
void printVoltage();
void printCurrent();
void measureVoltage();
void measureCurrent(Adafruit_INA219 &ina219, int i);
void toggleSource();
void switchBattery();
void switchPsu(bool warmup);
void shutdown();

} // namespace power
