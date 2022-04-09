#pragma once

#include "ITime.h"

namespace common {

class Log;

}

namespace native {
namespace greenhouse {

class ISystem {
public:
  virtual void ReportInfo(const char *m, ...) = 0;
  virtual void ReportWarning(const char *m, ...) = 0;
  virtual void ReportCritical(const char *m, ...) = 0;

  virtual void HandleNightToDayTransition() = 0;
  virtual void HandleDayToNightTransition() = 0;
  virtual void ReportWaterHeaterInfo() = 0;
  virtual float InsideAirTemperature() const = 0;
  virtual float SoilTemperature() const = 0;
  virtual float WaterTemperature() const = 0;
  virtual void SetSwitch(int i, bool on) = 0;
  virtual void Delay(unsigned long ms) = 0;

  virtual ITime &Time() = 0;
};

} // namespace greenhouse
} // namespace native
