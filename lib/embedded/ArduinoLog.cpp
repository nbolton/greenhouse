#include "ArduinoLog.h"

#include "ho_config.h"

#include <cstdarg>

static char s_buffer[80];

void ArduinoLog::TraceFlash(const __FlashStringHelper *f) const
{
  if (!k_trace) {
    return;
  }

  Serial.println(f);
}

void ArduinoLog::Trace(const char *format, ...) const
{
  if (!k_trace) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  vsprintf(s_buffer, format, args);
  va_end(args);

  Serial.println(s_buffer);
}
