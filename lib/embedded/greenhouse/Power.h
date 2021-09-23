#pragma once

#include "../../native/greenhouse/ISystem.h"
#include "../Log.h"
#include "ISystem.h"

namespace embedded {
namespace greenhouse {

enum PvModes { k_pvAuto, k_pvOn, k_pvOff };

class Power {
public:
  Power(int psuRelayPin, int pvRelayPin);
  const embedded::Log &Log() const { return m_log; }
  void Setup();
  void Loop();
  void ThreadCallback();
  void InitPowerSource();
  void MeasureVoltage();
  void MeasureCurrent();

public:
  // getters & setters
  void PvVoltageSensorMin(float value) { m_pvVoltageSensorMin = value; }
  void PvVoltageSensorMax(float value) { m_pvVoltageSensorMax = value; }
  void PvVoltageOutputMin(float value) { m_pvVoltageOutputMin = value; }
  void PvVoltageOutputMax(float value) { m_pvVoltageOutputMax = value; }
  void PvCurrentSensorMin(float value) { m_pvCurrentSensorMin = value; }
  void PvCurrentSensorMax(float value) { m_pvCurrentSensorMax = value; }
  void PvCurrentOutputMin(float value) { m_pvCurrentOutputMin = value; }
  void PvCurrentOutputMax(float value) { m_pvCurrentOutputMax = value; }
  void PvVoltageSwitchOn(float value) { m_pvVoltageSwitchOn = value; }
  void PvVoltageSwitchOff(float value) { m_pvVoltageSwitchOff = value; }
  bool PvPowerSource() { return m_pvPowerSource; }
  float PvVoltageSensor() { return m_pvVoltageSensor; }
  float PvVoltageOutput() { return m_pvVoltageOutput; }
  float PvCurrentSensor() { return m_pvCurrentSensor; }
  float PvCurrentOutput() { return m_pvCurrentOutput; }
  PvModes PvMode() const { return m_pvMode; }
  void PvMode(PvModes value) { m_pvMode = value; }
  embedded::greenhouse::ISystem &Embedded() const;
  void Embedded(embedded::greenhouse::ISystem &value) { m_embedded = &value; }
  native::greenhouse::ISystem &Native() const;
  void Native(native::greenhouse::ISystem &value) { m_native = &value; }

private:
  void SwitchPower(bool pv);

private:
  embedded::greenhouse::ISystem *m_embedded;
  native::greenhouse::ISystem *m_native;
  embedded::Log m_log;
  int m_psuRelayPin;
  int m_pvRelayPin;
  bool m_pvPowerSource;
  float m_pvVoltageSwitchOn;
  float m_pvVoltageSwitchOff;
  float m_pvVoltageSensor;
  float m_pvVoltageOutput;
  float m_pvVoltageSensorMin;
  float m_pvVoltageSensorMax;
  float m_pvVoltageOutputMin;
  float m_pvVoltageOutputMax;
  float m_pvCurrentSensor;
  float m_pvCurrentOutput;
  float m_pvCurrentSensorMin;
  float m_pvCurrentSensorMax;
  float m_pvCurrentOutputMin;
  float m_pvCurrentOutputMax;
  PvModes m_pvMode;
};

float readPvOnboardVoltage();

} // namespace greenhouse
} // namespace embedded