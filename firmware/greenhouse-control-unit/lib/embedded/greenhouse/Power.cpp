#include "Power.h"

#include "common.h"

#include <Arduino.h>

using namespace common;

namespace embedded {
namespace greenhouse {

#define VOLTAGE_DIFF_DELTA .1f
#define TRACE_SENSOR_RAW 0

const float k_localVoltageMin = 9.5;
const float k_localVoltageMapIn = 1.184;
const float k_localVoltageMapOut = 10.0;
const int k_switchDelay = 1000;  // delay between switching both relays
const bool k_relayTest = false; // useful for testing power drop
const int k_relayTestInterval = 2000;

static Power *s_instance = nullptr;
bool s_testState = false;

// member functions

Power::Power(int psuRelayPin, int batteryRelayPin, int batteryLedPin, int psuLedPin) :
  m_embedded(nullptr),
  m_native(nullptr),
  m_psuRelayPin(psuRelayPin),
  m_batteryRelayPin(batteryRelayPin),
  m_batteryLedPin(batteryLedPin),
  m_psuLedPin(psuLedPin),
  m_batteryPowerSource(false),
  m_batteryVoltageSwitchOn(k_unknown),
  m_batteryVoltageMin(k_unknown),
  m_batteryVoltageSensor(k_unknown),
  m_batteryVoltageOutput(k_unknown),
  m_batteryVoltageSensorMin(0),
  m_batteryVoltageSensorMax(k_unknown),
  m_batteryVoltageOutputMin(0),
  m_batteryVoltageOutputMax(k_unknown),
  m_batteryCurrentSensor(k_unknown),
  m_batteryCurrentOutput(k_unknown),
  m_batteryCurrentSensorMin(0),
  m_batteryCurrentSensorMax(k_unknown),
  m_batteryCurrentOutputMin(0),
  m_batteryCurrentOutputMax(k_unknown),
  m_lastLocalVoltage(k_unknown),
  //m_lastPsuVoltage(k_unknown),
  m_batteryMode(BatteryModes::k_batteryAuto)
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
  
  float localVoltage = ReadLocalVoltage();
  if (abs(m_lastLocalVoltage - localVoltage) > VOLTAGE_DIFF_DELTA) {
    TRACE_F("Local voltage changed: %.2fV", localVoltage);
  }
  m_lastLocalVoltage = localVoltage;

  if (localVoltage <= k_localVoltageMin) {
    if (m_batteryPowerSource) {
      Native().ReportWarning("Local voltage drop detected: %.2fV", localVoltage);
      SwitchPower(false);
    }
  }
  else if (BatteryMode() != BatteryModes::k_batteryAuto) {
    if (!m_batteryPowerSource && (BatteryMode() == BatteryModes::k_batteryOn)) {
      Native().ReportWarning("Battery mode is manual (battery on)");
      SwitchPower(true);
    }
    else if (m_batteryPowerSource && (BatteryMode() == BatteryModes::k_batteryOff)) {
      Native().ReportWarning("Battery mode is manual (battery off)");
      SwitchPower(false);
    }
  }
  else if (m_batteryVoltageOutput != k_unknown) {

    if (
      !m_batteryPowerSource && (m_batteryVoltageSwitchOn != k_unknown) &&
      (m_batteryVoltageOutput >= m_batteryVoltageSwitchOn)) {
      TRACE("Switching to battery automatically (PSU off)");
      SwitchPower(true);
    }
    else if (
      m_batteryPowerSource && (m_batteryVoltageMin != k_unknown) &&
      (m_batteryVoltageOutput <= m_batteryVoltageMin)) {
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
  MeasureVoltage();
  m_lastLocalVoltage = ReadLocalVoltage();
  TRACE_F(
    "Init power source, local=%.2fV (min=%.2fV), battery=%.2fV (min=%.2fV)",
    m_lastLocalVoltage,
    k_localVoltageMin,
    m_batteryVoltageOutput,
    m_batteryVoltageMin);

  bool batteryVoltageAboveMin = m_batteryVoltageOutput >= m_batteryVoltageMin;
  bool localVoltageAboveMin = m_lastLocalVoltage >= k_localVoltageMin;

  if ((m_batteryVoltageSwitchOn != k_unknown) && batteryVoltageAboveMin && localVoltageAboveMin) {

    TRACE("Using battery on start (onboard voltage and battery above min)");
    SwitchPower(true);
  }
  else {

    if (!batteryVoltageAboveMin) {
      TRACE("Battery voltage is below min");
    }

    if (!localVoltageAboveMin) {
      TRACE("Local voltage is below min");
    }

    TRACE("Using PSU on start");
    SwitchPower(false);
  }
}

void Power::MeasureVoltage()
{
  if (
    !Embedded().BatterySensorReady() || m_batteryVoltageSensorMax == k_unknown ||
    m_batteryVoltageOutputMax == k_unknown) {
    return;
  }

  m_batteryVoltageSensor = Embedded().ReadBatteryVoltageSensorRaw();
  m_batteryVoltageOutput = mapFloat(
    m_batteryVoltageSensor,
    m_batteryVoltageSensorMin,
    m_batteryVoltageSensorMax,
    m_batteryVoltageOutputMin,
    m_batteryVoltageOutputMax);
}

void Power::MeasureCurrent()
{
  if (
    !Embedded().BatterySensorReady() || m_batteryCurrentSensorMax == k_unknown ||
    m_batteryCurrentOutputMax == k_unknown) {
    return;
  }

  m_batteryCurrentSensor = Embedded().ReadBatteryCurrentSensorRaw();
  m_batteryCurrentOutput = mapFloat(
    m_batteryCurrentSensor,
    m_batteryCurrentSensorMin,
    m_batteryCurrentSensorMax,
    m_batteryCurrentOutputMin,
    m_batteryCurrentOutputMax);
}

void Power::SwitchPower(bool useBattery)
{
  // first, close both NC relays to ensure constant power
  Embedded().ShiftRegister(m_psuRelayPin, false);
  Embedded().ShiftRegister(m_batteryRelayPin, false);
  Embedded().ShiftRegister(m_psuLedPin, true);
  Embedded().ShiftRegister(m_batteryLedPin, true);

  // if on battery, give the PSU time to power up, or,
  // if on PSU, allow the battery relay to close first.
  Embedded().Delay(k_switchDelay, "Relay");

  if (!useBattery) {
    TRACE("PSU AC NC relay closed (PSU on), battery NC relay open (battery off)");
  }
  else {
    TRACE("Battery NC relay closed (battery on), PSU AC NC relay open (PSU off)");
  }

  // false = closed (on the NC relay)
  Embedded().ShiftRegister(m_psuRelayPin, useBattery);
  Embedded().ShiftRegister(m_batteryRelayPin, !useBattery);

  Embedded().ShiftRegister(m_psuLedPin, !useBattery);
  Embedded().ShiftRegister(m_batteryLedPin, useBattery);

  m_batteryPowerSource = useBattery;
  TRACE_F("Source: %s", useBattery ? "Battery" : "PSU");
  TRACE_F("Local voltage: %.2fV", ReadLocalVoltage());
  Embedded().OnPowerSwitch();
}

float Power::ReadLocalVoltage()
{
  float f = Embedded().ReadLocalVoltageSensorRaw();
  if (f == k_unknown) {
    return k_unknown;
  }
#if TRACE_SENSOR_RAW
  TRACE_F("Local voltage sensor raw: %.4f", f);
#endif // TRACE_SENSOR_RAW
  return mapFloat(f, 0, k_localVoltageMapIn, 0, k_localVoltageMapOut);
}

// free function definitions

} // namespace greenhouse
} // namespace embedded