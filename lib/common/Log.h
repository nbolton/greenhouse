#pragma once

class Log {
public:
  virtual void Trace(const char *m, ...) const;
};
