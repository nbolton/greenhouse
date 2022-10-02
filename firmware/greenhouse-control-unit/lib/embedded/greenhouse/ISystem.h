#pragma once

namespace embedded {
namespace greenhouse {

class ISystem {
public:
  virtual void Delay(unsigned long ms, const char* reason) = 0;
  virtual void OnPowerSwitch() = 0;
  virtual void ExpanderWrite(int pin, int value) = 0;
  virtual void ShiftRegister(int pin, bool set) = 0;
  virtual bool BatterySensorReady() = 0;
  virtual float ReadBatteryVoltageSensorRaw() = 0;
  virtual float ReadBatteryCurrentSensorRaw() = 0;
  virtual float ReadLocalVoltageSensorRaw() = 0;
  //virtual float ReadPsuVoltageSensor() = 0;
};

} // namespace greenhouse
} // namespace embedded