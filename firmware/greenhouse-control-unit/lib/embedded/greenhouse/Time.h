#pragma once

#include "../../native/greenhouse/Time.h"
#include "../../common/log.h"

#include <Arduino.h>

namespace embedded {
namespace greenhouse {

class Time : public native::greenhouse::Time {

public:
  Time();
  void Setup();
  void Refresh();
  int CurrentHour() const;
  unsigned long EpochTime() const;
  unsigned long UptimeSeconds() const;
  String FormattedUptime();
  String FormattedCurrentTime();

private:
  bool m_timeClientOk;
};

} // namespace greenhouse
} // namespace embedded
