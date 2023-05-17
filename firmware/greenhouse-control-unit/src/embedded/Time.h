#pragma once

#include "log.h"
#include "native/Time.h"

#include <Arduino.h>

namespace greenhouse {
namespace embedded {

class Time : public greenhouse::native::Time {

public:
  Time();
  void Setup();
  void Refresh();
  unsigned long UptimeSeconds() const;
  int CurrentHour() const;
  unsigned long EpochTime() const;
  String FormattedUptime();
  String FormattedCurrentTime();
  bool IsValid() const { return m_success > 0; }

private:
  void update(int maxRetry);

private:
  bool m_timeClientOk;
  int m_success;
  int m_errors;
};

} // namespace embedded
} // namespace greenhouse
