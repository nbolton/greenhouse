#include "ArduinoLog.h"

#include <cstdarg>

static char buffer[80];

void ArduinoLog::TraceFlash(const __FlashStringHelper *f) const
{
  Serial.println(f);
}

void ArduinoLog::Trace(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  Serial.println(buffer);
}
