#pragma once

namespace embedded {
namespace greenhouse {

class ISystem {
public:
  virtual void Delay(unsigned long ms, const char* reason) = 0;
  virtual void OnPowerSwitch() = 0;
  virtual void WriteIO(uint8_t pin, uint8_t value) = 0;
  virtual bool BatterySensorReady() = 0;
  virtual float ReadBatteryVoltageSensorRaw() = 0;
  virtual float ReadBatteryCurrentSensorRaw() = 0;
};

} // namespace greenhouse
} // namespace embedded