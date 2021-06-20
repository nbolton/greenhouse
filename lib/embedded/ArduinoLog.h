#pragma once

#include "Log.h"

class ArduinoLog : public Log
{
  public:
    void Trace(const char *m, ...) const;
    void ReportInfo(const char *m, ...) const;
    void ReportWarning(const char *m, ...) const;
    void ReportCritical(const char *m, ...) const;
};
