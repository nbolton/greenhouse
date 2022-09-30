#include "Power.h"

#include "common.h"

#include <Arduino.h>

using namespace common;

namespace embedded {
namespace greenhouse {

#define VOLTAGE_DIFF_DELTA .1f

const float k_commonVoltageMin = 9.5;
const float k_commonVoltageMapIn = 2.10;
const float k_commonVoltageMapOut = 12.0;
#if 0
const float k_psuVoltageMin = 9.5;
const float k_psuVoltageMapIn = 2.10;
const float k_psuVoltageMapOut = 12.0;
#endif
const int k_switchDelay = 1000;  // delay between switching both relays
const bool k_relayTest = false; // useful for testing power drop
const int k_relayTestInterval = 2000;

static Power *s_instance = nullptr;
bool s_testState = false;

// member functions

Power::Power(int psuRelayPin, int pvRelayPin, int batteryLedPin, int psuLedPin) :
  m_embedded(nullptr),
  m_native(nullptr),
  m_psuRelayPin(psuRelayPin),
  m_pvRelayPin(pvRelayPin),
  m_batteryLedPin(batteryLedPin),
  m_psuLedPin(psuLedPin),
  m_pvPowerSource(false),
  m_pvVoltageSwitchOn(k_unknown),
  m_pvVoltageSwitchOff(k_unknown),
  m_pvVoltageSensor(k_unknown),
  m_pvVoltageOutput(k_unknown),
  m_pvVoltageSensorMin(0),
  m_pvVoltageSensorMax(k_unknown),
  m_pvVoltageOutputMin(0),
  m_pvVoltageOutputMax(k_unknown),
  m_pvCurrentSensor(k_unknown),
  m_pvCurrentOutput(k_unknown),
  m_pvCurrentSensorMin(0),
  m_pvCurrentSensorMax(k_unknown),
  m_pvCurrentOutputMin(0),
  m_pvCurrentOutputMax(k_unknown),
  m_lastCommonVoltage(k_unknown),
  //m_lastPsuVoltage(k_unknown),
  m_pvMode(PvModes::k_pvAuto)
{
}

void Power::Setup()
{
  s_instance = this;

  // circuit default state is both power sources connected
  Embedded().ShiftRegister(m_psuLedPin, true);
  Embedded().ShiftRegister(m_batteryLedPin, true);
}

void Power::Loop()
{
  MeasureVoltage();
  
  float commonVoltage = ReadCommonVoltage();
  if (abs(m_lastCommonVoltage - commonVoltage) > VOLTAGE_DIFF_DELTA) {
    TRACE_F("Local voltage changed: %.2fV", commonVoltage);
  }
  m_lastCommonVoltage = commonVoltage;
  
  #if 0
  float psuVoltage = ReadPsuVoltage();
  if (abs(m_lastPsuVoltage - psuVoltage) > VOLTAGE_DIFF_DELTA) {
    TRACE_F("PSU voltage changed: %.2fV", psuVoltage);
  }
  m_lastPsuVoltage = psuVoltage;
  #endif

  if (commonVoltage <= k_commonVoltageMin) {
    if (m_pvPowerSource) {
      Native().ReportWarning("Local voltage drop detected: %.2fV", commonVoltage);
      SwitchPower(false);
    }
  }
  else if (PvMode() != PvModes::k_pvAuto) {
    if (!m_pvPowerSource && (PvMode() == PvModes::k_pvOn)) {
      Native().ReportWarning("Battery mode is manual (battery on)");
      SwitchPower(true);
    }
    else if (m_pvPowerSource && (PvMode() == PvModes::k_pvOff)) {
      Native().ReportWarning("Battery mode is manual (battery off)");
      SwitchPower(false);
    }
  }
  else if (m_pvVoltageOutput != k_unknown) {

    if (
      !m_pvPowerSource && (m_pvVoltageSwitchOn != k_unknown) &&
      (m_pvVoltageOutput >= m_pvVoltageSwitchOn)) {
      TRACE("Switching to battery automatically (PSU off)");
      SwitchPower(true);
    }
    else if (
      m_pvPowerSource && (m_pvVoltageSwitchOff != k_unknown) &&
      (m_pvVoltageOutput <= m_pvVoltageSwitchOff)) {
      TRACE("Switching PSU on automatically (battery off)");
      SwitchPower(false);
    }
  }
}

native::greenhouse::ISystem &Power::Native() const
{
  if (m_native == nullptr) {
    TRACE("Native system not set");
    throw;
  }
  return *m_native;
}

embedded::greenhouse::ISystem &Power::Embedded() const
{
  if (m_embedded == nullptr) {
    TRACE("Embedded system not set");
    throw;
  }
  return *m_embedded;
}

void Power::InitPowerSource()
{
  const float sensorMin = m_pvVoltageSwitchOff;

  MeasureVoltage();
  m_lastCommonVoltage = ReadCommonVoltage();
  //m_lastPsuVoltage = ReadPsuVoltage();
  TRACE_F(
    "Init power source, common=%.2fV, psu=%.2fV, battery=%.2fV, min=%.2fV",
    m_lastCommonVoltage,
    //m_lastPsuVoltage,
    m_lastCommonVoltage,
    m_pvVoltageOutput,
    sensorMin);

  bool pvSensorAboveMin = m_pvVoltageOutput >= sensorMin;
  bool commonAboveMin = m_lastCommonVoltage >= k_commonVoltageMin;
  //bool psuAboveMin = m_lastPsuVoltage >= k_psuVoltageMin;
  bool psuAboveMin = true;

  if ((m_pvVoltageSwitchOn != k_unknown) && pvSensorAboveMin && commonAboveMin) {

    TRACE("Using battery on start (onboard voltage and battery above min)");
    SwitchPower(true);
  }
  else if (psuAboveMin) {

    if (!pvSensorAboveMin) {
      TRACE("Battery voltage is below min");
    }

    if (!commonAboveMin) {
      TRACE("Local voltage is below min");
    }

    TRACE("Using PSU on start");
    SwitchPower(false);
  }
  else {
    TRACE("Unable to use PSU, no voltage");
  }
}

void Power::MeasureVoltage()
{
  if (
    !Embedded().PowerSensorReady() || m_pvVoltageSensorMax == k_unknown ||
    m_pvVoltageOutputMax == k_unknown) {
    return;
  }

  m_pvVoltageSensor = Embedded().ReadPowerSensorVoltage();
  m_pvVoltageOutput = mapFloat(
    m_pvVoltageSensor,
    m_pvVoltageSensorMin,
    m_pvVoltageSensorMax,
    m_pvVoltageOutputMin,
    m_pvVoltageOutputMax);
}

void Power::MeasureCurrent()
{
  if (
    !Embedded().PowerSensorReady() || m_pvCurrentSensorMax == k_unknown ||
    m_pvCurrentOutputMax == k_unknown) {
    return;
  }

  m_pvCurrentSensor = Embedded().ReadPowerSensorCurrent();
  m_pvCurrentOutput = mapFloat(
    m_pvCurrentSensor,
    m_pvCurrentSensorMin,
    m_pvCurrentSensorMax,
    m_pvCurrentOutputMin,
    m_pvCurrentOutputMax);
}

void Power::SwitchPower(bool pv)
{
  // first, close both NC relays to ensure constant power
  Embedded().ShiftRegister(m_psuRelayPin, false);
  Embedded().ShiftRegister(m_pvRelayPin, false);
  Embedded().ShiftRegister(m_psuLedPin, true);
  Embedded().ShiftRegister(m_batteryLedPin, true);

  // if on battery, give the PSU time to power up, or,
  // if on PSU, allow the battery relay to close first.
  Embedded().Delay(k_switchDelay, "Relay");

  if (!pv) {
    TRACE("PSU AC NC relay closed (PSU on), battery NC relay open (battery off)");
  }
  else {
    TRACE("Battery NC relay closed (battery on), PSU AC NC relay open (PSU off)");
  }

  // false = closed (on the NC relay)
  Embedded().ShiftRegister(m_psuRelayPin, pv);
  Embedded().ShiftRegister(m_pvRelayPin, !pv);

  Embedded().ShiftRegister(m_psuLedPin, !pv);
  Embedded().ShiftRegister(m_batteryLedPin, pv);

  m_pvPowerSource = pv;
  TRACE_F("Source: %s", pv ? "Battery" : "PSU");
  Embedded().OnPowerSwitch();
}

float Power::ReadCommonVoltage()
{
  float f = Embedded().ReadCommonVoltageSensor();
  if (f == k_unknown) {
    return k_unknown;
  }
  return mapFloat(f, 0, k_commonVoltageMapIn, 0, k_commonVoltageMapOut);
}

#if 0
float Power::ReadPsuVoltage()
{
  float f = Embedded().ReadPsuVoltageSensor();
  if (f == k_unknown) {
    return k_unknown;
  }
  return mapFloat(f, 0, k_psuVoltageMapIn, 0, k_psuVoltageMapOut);
}
#endif

// free function definitions

} // namespace greenhouse
} // namespace embedded