#include "ArduinoLog.h"

#include "BlynkCommon.h"

#include <cstdarg>
#include <iostream>

char buffer[80];

void ArduinoLog::Trace(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  Serial.println(buffer);
}

void ArduinoLog::ReportInfo(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  
  Blynk.logEvent("log_info", buffer);
}

void ArduinoLog::ReportWarning(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  
  Blynk.logEvent("log_warning", buffer);
}

void ArduinoLog::ReportCritical(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  
  Blynk.logEvent("log_critical", buffer);
}
