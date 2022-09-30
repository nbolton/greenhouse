#pragma once

#if TRACE_EN
#if ARDUINO

#include <Arduino.h>

#define TRACE(line) common::log::traceFlash(F(line))
#define TRACE_F(format, ...) common::log::traceFlash(F(format), __VA_ARGS__)
#define TRACE_C(line) common::log::trace(line)

#else

#define TRACE(line) common::log::trace(line)
#define TRACE_F(format, ...) common::log::trace(format, __VA_ARGS__)
#define TRACE_C(line) TRACE(line)

#endif // ARDUINO

namespace common {
namespace log {

#if ARDUINO
void traceFlash(const __FlashStringHelper *format, ...);
#endif // ARDUINO

void trace(const char *format, ...);

} // namespace log
} // namespace common

#else

#define TRACE(f)
#define TRACE_F(f, ...)

#endif // TRACE_EN
