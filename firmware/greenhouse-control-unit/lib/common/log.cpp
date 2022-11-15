#if TRACE_EN

#include "log.h"

#if ARDUINO
#include <Arduino.h>
#endif // ARDUINO

#include "gh_config.h"

#include <cstdarg>
#include <iostream>
#include <string.h>

namespace common {
namespace log {

char s_buffer[200];

#if ARDUINO

int s_timeLen = 0;

void traceFlash(const __FlashStringHelper *format, ...)
{
  if (!k_trace) {
    return;
  }

  sprintf(s_buffer, "[%lu] ", millis());
  s_timeLen = strlen(s_buffer);

  char* buf = s_buffer + s_timeLen;
  int len = sizeof(s_buffer) - s_timeLen;

  va_list args;
  va_start(args, (PGM_P)format);
  vsnprintf_P(buf, len, (const char *)format, args);
  va_end(args);

  Serial.println(s_buffer);
}

#endif // ARDUINO

void trace(const char *format, ...)
{
  if (!k_trace) {
    return;
  }
  
  char* buf = s_buffer;

#if ARDUINO

  sprintf(s_buffer, "[%lu] ", millis());
  s_timeLen = strlen(s_buffer);

  buf += s_timeLen;

#endif // ARDUINO

  va_list args;
  va_start(args, format);
  vsprintf(buf, format, args);
  va_end(args);

#if ARDUINO
  Serial.println(s_buffer);
#else
  printf("%s\n", s_buffer);
#endif // ARDUINO
}

} // namespace log
} // namespace common

#endif // TRACE_EN
