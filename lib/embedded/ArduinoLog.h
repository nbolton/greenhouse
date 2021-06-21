#pragma once

#include "Log.h"

#include <Arduino.h>

class ArduinoLog : public Log {
public:
  void TraceFlash(const __FlashStringHelper *f) const;
  void Trace(const char *m, ...) const;
};
