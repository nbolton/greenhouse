#include "../embedded/Log.h"

#include "ho_config.h"

#include <cstdarg>
#include <Arduino.h>

namespace embedded {
  
static char s_buffer[200];

void Log::Trace(const char *format, ...) const
{
  if (!k_trace) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  vsprintf(s_buffer, format, args);
  va_end(args);

  Serial.println("---> format: " + String(format));
  Serial.println(s_buffer);
}

void Log::Trace(const __FlashStringHelper *format, ...) const
{
  if (!k_trace) {
    return;
  }
  
  va_list args;
  va_start(args, (PGM_P)format);
  vsnprintf_P(s_buffer, sizeof(s_buffer), (const char *)format, args);
  va_end(args);

  Serial.println("---> format: " + String(format));
  Serial.println(s_buffer);
}

}
