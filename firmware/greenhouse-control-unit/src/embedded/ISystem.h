#pragma once

#include <Arduino.h>

namespace greenhouse {
namespace embedded {

class ISystem {
public:
  virtual void Delay(unsigned long ms, const char *reason) = 0;
  virtual void OnPowerSwitch() = 0;
  virtual void WriteOnboardIO(uint8_t pin, uint8_t value) = 0;
  virtual void ReportPumpSwitch(bool pumpOn) = 0;
  virtual void ReportPumpStatus(String message) = 0;
  virtual void OnBatteryCurrentChange() = 0;
};

} // namespace embedded
} // namespace greenhouse