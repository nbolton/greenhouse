#pragma once

#if TRACE_EN
#if ARDUINO

#include <Arduino.h>

#define TRACE(line) greenhouse::log::traceFlash(F(line))
#define TRACE_F(format, ...) greenhouse::log::traceFlash(F(format), __VA_ARGS__)
#define TRACE_C(line) greenhouse::log::trace(line)
#define BOOL_FS(b) String(b ? F("true") : F("false")).c_str()

#else

#define TRACE(line) greenhouse::log::trace(line)
#define TRACE_F(format, ...) greenhouse::log::trace(format, __VA_ARGS__)
#define TRACE_C(line) TRACE(line)

#endif // ARDUINO

namespace greenhouse {
namespace log {

#if ARDUINO
void traceFlash(const __FlashStringHelper *format, ...);
#endif // ARDUINO

void trace(const char *format, ...);

} // namespace log
} // namespace greenhouse

#else

#define TRACE(f)
#define TRACE_F(f, ...)

#endif // TRACE_EN
