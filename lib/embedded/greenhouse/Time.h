#pragma once

#include "../../native/greenhouse/Time.h"
#include "../embedded/Log.h"

#include <Arduino.h>

namespace embedded {
namespace greenhouse {

class Time : public native::greenhouse::Time {

public:
  Time();
  void Setup();
  void Refresh();
  const embedded::Log &Log() const { return m_log; }
  int CurrentHour() const;
  unsigned long EpochTime() const;
  unsigned long UptimeSeconds() const;
  String FormattedUptime();
  String FormattedCurrentTime();

private:
  embedded::Log m_log;
  bool m_timeClientOk;
};

} // namespace greenhouse
} // namespace embedded
