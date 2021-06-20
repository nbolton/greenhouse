#include "Log.h"
#include <cstdarg>
#include <iostream>

void Log::Trace(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}
