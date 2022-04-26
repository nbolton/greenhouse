#pragma once

namespace embedded {
namespace greenhouse {

class ISystem {
public:
  virtual void Delay(unsigned long ms, const char* reason) = 0;
  virtual void OnPowerSwitch() = 0;
  virtual void ExpanderWrite(int pin, int value) = 0;
  virtual void ShiftRegister(int pin, bool set) = 0;
  virtual bool PowerSensorReady() = 0;
  virtual float ReadPowerSensorVoltage() = 0;
  virtual float ReadPowerSensorCurrent() = 0;
  virtual float ReadCommonVoltageSensor() = 0;
  virtual float ReadPsuVoltageSensor() = 0;
  virtual void QueueRefresh() = 0;
};

} // namespace greenhouse
} // namespace embedded