#pragma once

#include "Log.h"

#include <Arduino.h>

class ArduinoLog : public Log {
public:
  void Trace(const char *format, ...) const;
  void Trace(const __FlashStringHelper *format, ...) const;
};
