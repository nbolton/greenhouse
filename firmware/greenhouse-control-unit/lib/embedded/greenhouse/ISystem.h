#pragma once

#include <Arduino.h>

namespace embedded {
namespace greenhouse {

class ISystem {
public:
  virtual void Delay(unsigned long ms, const char *reason) = 0;
  virtual void OnPowerSwitch() = 0;
  virtual void WriteOnboardIO(uint8_t pin, uint8_t value) = 0;
  virtual void LowerPumpOn(bool pumpOn) = 0;
  virtual void LowerPumpStatus(const char *message) = 0;
  virtual void OnBatteryCurrentChange() = 0;
};

} // namespace greenhouse
} // namespace embedded