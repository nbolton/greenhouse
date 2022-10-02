#pragma once

#include "../../native/greenhouse/ISystem.h"
#include "../../common/log.h"
#include "ISystem.h"

namespace embedded {
namespace greenhouse {

enum BatteryModes { k_batteryAuto, k_batteryOn, k_batteryOff };

class Power {
public:
  Power(int psuRelayPin, int batteryRelayPin, int batteryLedPin, int psuLedPin);
  void Setup();
  void Loop();
  void InitPowerSource();
  void MeasureVoltage();
  void MeasureCurrent();

public:
  // getters & setters
  void BatteryVoltageSensorMin(float value) { m_batteryVoltageSensorMin = value; }
  void BatteryVoltageSensorMax(float value) { m_batteryVoltageSensorMax = value; }
  void BatteryVoltageOutputMin(float value) { m_batteryVoltageOutputMin = value; }
  void BatteryVoltageOutputMax(float value) { m_batteryVoltageOutputMax = value; }
  void BatteryCurrentSensorMin(float value) { m_batteryCurrentSensorMin = value; }
  void BatteryCurrentSensorMax(float value) { m_batteryCurrentSensorMax = value; }
  void BatteryCurrentOutputMin(float value) { m_batteryCurrentOutputMin = value; }
  void BatteryCurrentOutputMax(float value) { m_batteryCurrentOutputMax = value; }
  void BatteryVoltageSwitchOn(float value) { m_batteryVoltageSwitchOn = value; }
  void BatteryVoltageSwitchOff(float value) { m_batteryVoltageSwitchOff = value; }
  bool BatteryPowerSource() { return m_batteryPowerSource; }
  float BatteryVoltageSensor() { return m_batteryVoltageSensor; }
  float BatteryVoltageOutput() { return m_batteryVoltageOutput; }
  float BatteryCurrentSensor() { return m_batteryCurrentSensor; }
  float BatteryCurrentOutput() { return m_batteryCurrentOutput; }
  BatteryModes BatteryMode() const { return m_batteryMode; }
  void BatteryMode(BatteryModes value) { m_batteryMode = value; }
  embedded::greenhouse::ISystem &Embedded() const;
  void Embedded(embedded::greenhouse::ISystem &value) { m_embedded = &value; }
  native::greenhouse::ISystem &Native() const;
  void Native(native::greenhouse::ISystem &value) { m_native = &value; }
  float ReadLocalVoltage();
  //float ReadPsuVoltage();

private:
  void SwitchPower(bool useBattery);

private:
  embedded::greenhouse::ISystem *m_embedded;
  native::greenhouse::ISystem *m_native;
  int m_psuRelayPin;
  int m_batteryRelayPin;
  int m_batteryLedPin;
  int m_psuLedPin;
  bool m_batteryPowerSource;
  float m_batteryVoltageSwitchOn;
  float m_batteryVoltageSwitchOff;
  float m_batteryVoltageSensor;
  float m_batteryVoltageOutput;
  float m_batteryVoltageSensorMin;
  float m_batteryVoltageSensorMax;
  float m_batteryVoltageOutputMin;
  float m_batteryVoltageOutputMax;
  float m_batteryCurrentSensor;
  float m_batteryCurrentOutput;
  float m_batteryCurrentSensorMin;
  float m_batteryCurrentSensorMax;
  float m_batteryCurrentOutputMin;
  float m_batteryCurrentOutputMax;
  float m_lastLocalVoltage;
  //float m_lastPsuVoltage;
  BatteryModes m_batteryMode;
};

} // namespace greenhouse
} // namespace embedded