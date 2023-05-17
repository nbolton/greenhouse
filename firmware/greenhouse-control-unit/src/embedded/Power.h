#pragma once

#include "ISystem.h"
#include "common.h"
#include "log.h"
#include "native/ISystem.h"

namespace greenhouse {
namespace embedded {

enum PowerSource { k_powerSourceBoth, k_powerSourceBattery, k_powerSourcePsu };

enum PowerMode {
  k_powerModeUnknown = k_unknown,
  k_powerModeAuto = 0,
  k_powerModeManualBattery = 1,
  k_powerModeManualPsu = 2
};

class Power {
public:
  Power();
  void Setup();
  void Loop();
  void InitPowerSource();
  void MeasureVoltage();
  void MeasureCurrent();

public:
  // getters & setters
  PowerMode Mode() const { return m_mode; }
  void Mode(PowerMode value)
  {
    m_mode = value;
    m_nextSwitch = k_unknownUL;
  }
  PowerSource Source() const { return m_source; }
  void BatteryVoltageSwitchOn(float value) { m_batteryVoltageSwitchOn = value; }
  void BatteryVoltageSwitchOff(float value) { m_batteryVoltageSwitchOff = value; }
  float BatteryVoltageSensor() { return m_batteryVoltageSensor; }
  float BatteryVoltageOutput() { return m_batteryVoltageOutput; }
  float BatteryCurrentSensor() { return m_batteryCurrentSensor; }
  float BatteryCurrentOutput() { return m_batteryCurrentOutput; }
  greenhouse::embedded::ISystem &Embedded() const;
  void Embedded(greenhouse::embedded::ISystem &value) { m_embedded = &value; }
  greenhouse::native::ISystem &Native() const;
  void Native(greenhouse::native::ISystem &value) { m_native = &value; }

private:
  void switchSource(PowerSource source);
  bool batteryIsLow() const;
  bool batteryIsCharged() const;

private:
  greenhouse::embedded::ISystem *m_embedded;
  greenhouse::native::ISystem *m_native;
  PowerMode m_mode;
  PowerSource m_source;
  float m_batteryVoltageSwitchOn;
  float m_batteryVoltageSwitchOff;
  float m_batteryVoltageSensor;
  float m_batteryVoltageOutput;
  float m_batteryCurrentSensor;
  float m_batteryCurrentOutput;
  float m_lastBatteryVoltage;
  float m_lastBatteryCurrent;
  float m_psuVoltageSensor;
  float m_psuVoltageOutput;
  float m_lastPsuVoltage;
  float m_lastPsuCurrent;
  float m_lastMeasure;
  unsigned long m_nextSwitch;
};

} // namespace embedded
} // namespace greenhouse