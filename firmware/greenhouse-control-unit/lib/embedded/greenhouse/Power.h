#pragma once

#include "../../common/common.h"
#include "../../common/log.h"
#include "../../native/greenhouse/ISystem.h"
#include "ISystem.h"

namespace embedded {
namespace greenhouse {

enum PowerSource { k_powerSourceBoth, k_powerSourceBattery, k_powerSourcePsu };

enum PowerMode {
  k_powerModeUnknown = common::k_unknown,
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
    m_nextSwitch = common::k_unknownUL;
  }
  PowerSource Source() const { return m_source; }
  void BatteryVoltageSwitchOn(float value) { m_batteryVoltageSwitchOn = value; }
  void BatteryVoltageSwitchOff(float value) { m_batteryVoltageSwitchOff = value; }
  float BatteryVoltageSensor() { return m_batteryVoltageSensor; }
  float BatteryVoltageOutput() { return m_batteryVoltageOutput; }
  float BatteryCurrentSensor() { return m_batteryCurrentSensor; }
  float BatteryCurrentOutput() { return m_batteryCurrentOutput; }
  embedded::greenhouse::ISystem &Embedded() const;
  void Embedded(embedded::greenhouse::ISystem &value) { m_embedded = &value; }
  native::greenhouse::ISystem &Native() const;
  void Native(native::greenhouse::ISystem &value) { m_native = &value; }

private:
  void switchSource(PowerSource source);
  bool batteryIsLow() const;
  bool batteryIsCharged() const;

private:
  embedded::greenhouse::ISystem *m_embedded;
  native::greenhouse::ISystem *m_native;
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

} // namespace greenhouse
} // namespace embedded