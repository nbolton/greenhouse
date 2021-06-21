#include "Log.h"
#include <cstdarg>
#include <iostream>

char buffer[80];

void Log::Trace(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  printf("%s\n", buffer);
}
