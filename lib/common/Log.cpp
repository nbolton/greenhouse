#include "Log.h"

#include "ho_config.h"

#include <cstdarg>
#include <iostream>

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

  printf("%s\n", s_buffer);
}
