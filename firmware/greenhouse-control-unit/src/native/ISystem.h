#pragma once

#include "ITime.h"

namespace common {

class Log;

}

namespace greenhouse {
namespace native {

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
  virtual void Delay(unsigned long ms, const char *reason) = 0;

  virtual ITime &Time() = 0;
};

} // namespace native
} // namespace greenhouse
