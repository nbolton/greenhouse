#pragma once

#if TRACE_EN

#define TRACE_PRINT 0
#define TRACE_ERROR 1
#define TRACE_WARN 2
#define TRACE_INFO 3
#define TRACE_DEBUG1 4
#define TRACE_DEBUG2 5

#include <Arduino.h>

#define TRACE(level, line) shared::trace(level, F(__FILE__), __LINE__, F(line))
#define TRACE_F(level, format, ...)                                                                \
  shared::trace(level, F(__FILE__), __LINE__, F(format), __VA_ARGS__)
#define TRACE_C(level, line) TRACE_F(level, "%s", line)
#define FCSTR(str) String(F(str)).c_str()
#define BOOL_FS(b) String(b ? F("true") : F("false")).c_str()

namespace shared {

void traceInit();
void traceLevel(int level);
const char *traceLevelCStr();
const char *traceLevelCStr(int level);
void trace(
  int level, const __FlashStringHelper *file, int line, const __FlashStringHelper *format, ...);

} // namespace shared

#else

#define TRACE(line)
#define TRACE_VA(format, ...)

#endif // TRACE_ON
