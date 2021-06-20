#pragma once

class Log
{  
  public:
    virtual void Trace(const char *m, ...) const;
    virtual void ReportInfo(const char *m, ...) const { };
    virtual void ReportWarning(const char *m, ...) const { };
    virtual void ReportCritical(const char *m, ...) const { };
};
