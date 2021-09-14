#pragma once

namespace common {

class Log {
public:
  virtual void Trace(const char *format, ...) const;
};

} // namespace common
