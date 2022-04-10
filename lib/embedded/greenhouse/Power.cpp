#include "Power.h"

#include "common.h"

#include <Thread.h>

using namespace common;

namespace embedded {
namespace greenhouse {

const int s_threadInterval = 10000; // 10s
const int k_commonVoltageMin = 10;
const int k_psuVoltageMin = 10;
const float k_commonVoltageMapIn = 2.10;
const float k_commonVoltageMapOut = 12.0;
const float k_psuVoltageMapIn = 2.10;
const float k_psuVoltageMapOut = 12.0;
const int k_switchDelay = 500;  // delay between switching both relays
const bool k_relayTest = false; // useful for testing power drop
const int k_relayTestInterval = 2000;

static Power *s_instance = nullptr;
static Thread s_powerThread = Thread();
static Thread s_relayTestThread = Thread();
bool s_testState = false;

// free-functions declarations

void threadCallback() { s_instance->ThreadCallback(); }
void relayTestCallback() { s_instance->RelayTest(); }

// member functions

Power::Power(
  int psuRelayPin,
  int pvRelayPin,
  int batteryLedPin,
  int psuLedPin) :
  m_embedded(nullptr),
  m_native(nullptr),
  m_log(),
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
  m_pvMode(PvModes::k_pvAuto)
{
}

void Power::Setup()
{
  s_instance = this;
  s_powerThread.onRun(threadCallback);
  s_powerThread.setInterval(s_threadInterval);

  s_relayTestThread.onRun(relayTestCallback);
  s_relayTestThread.setInterval(k_relayTestInterval);

  // circuit default state is both power sources connected
  Embedded().ShiftRegister(m_psuLedPin, true);
  Embedded().ShiftRegister(m_batteryLedPin, true);
}

void Power::Loop()
{
  if (s_powerThread.shouldRun()) {
    s_powerThread.run();
  }

  if (k_relayTest && s_relayTestThread.shouldRun()) {
    s_relayTestThread.run();
  }
}

native::greenhouse::ISystem &Power::Native() const
{
  if (m_native == nullptr) {
    Log().Trace(F("Native system not set"));
    throw;
  }
  return *m_native;
}

embedded::greenhouse::ISystem &Power::Embedded() const
{
  if (m_embedded == nullptr) {
    Log().Trace(F("Embedded system not set"));
    throw;
  }
  return *m_embedded;
}

void Power::InitPowerSource()
{
  const float sensorMin = m_pvVoltageSwitchOff;

  MeasureVoltage();
  float commonVoltage = ReadCommonVoltage();
  float psuVoltage = ReadPsuVoltage();
  Log().Trace(
    F("Init power source, common=%.2fV, psu=%.2fV, pv=%.2fV, min=%.2fV"),
    commonVoltage,
    psuVoltage,
    m_pvVoltageOutput,
    sensorMin);

  bool pvSensorAboveMin = m_pvVoltageOutput >= sensorMin;
  bool commonAboveMin = commonVoltage >= k_commonVoltageMin;
  bool psuAboveMin = psuVoltage >= k_psuVoltageMin;

  if ((m_pvVoltageSwitchOn != k_unknown) && pvSensorAboveMin && commonAboveMin) {

    Log().Trace(F("Using PV on start (onboard voltage and PV above min)"));
    SwitchPower(true);
  }
  else if (psuAboveMin) {

    if (!pvSensorAboveMin) {
      Log().Trace(F("PV voltage is below min"));
    }

    if (!commonAboveMin) {
      Log().Trace(F("Common voltage is below min"));
    }

    Log().Trace(F("Using PSU on start"));
    SwitchPower(false);
  }
  else {
      Log().Trace(F("Unable to use PSU, no voltage"));
  }
}

void Power::ThreadCallback()
{
  MeasureVoltage();
  float commonVoltage = ReadCommonVoltage();

  if (commonVoltage <= k_commonVoltageMin) {
    if (m_pvPowerSource) {
      Native().ReportWarning("Common voltage drop detected: %.2fV", commonVoltage);
      SwitchPower(false);
    }
  }
  else if (PvMode() != PvModes::k_pvAuto) {
    if (!m_pvPowerSource && (PvMode() == PvModes::k_pvOn)) {
      Native().ReportWarning("PV mode is manual (PV on)");
      SwitchPower(true);
    }
    else if (m_pvPowerSource && (PvMode() == PvModes::k_pvOff)) {
      Native().ReportWarning("PV mode is manual (PV off)");
      SwitchPower(false);
    }
  }
  else if (m_pvVoltageOutput != k_unknown) {

    if (
      !m_pvPowerSource && (m_pvVoltageSwitchOn != k_unknown) &&
      (m_pvVoltageOutput >= m_pvVoltageSwitchOn)) {
      Log().Trace(F("Switching PV on automatically (PSU off)"));
      SwitchPower(true);
    }
    else if (
      m_pvPowerSource && (m_pvVoltageSwitchOff != k_unknown) &&
      (m_pvVoltageOutput <= m_pvVoltageSwitchOff)) {
      Log().Trace(F("Switching PSU on automatically (PV off)"));
      SwitchPower(false);
    }
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
  Embedded().Delay(k_switchDelay);

  if (!pv) {
    Log().Trace(F("PSU AC NC relay closed (PSU on), PV NC relay open (battery off)"));
  }
  else {
    Log().Trace(F("PV NC relay closed (battery on), PSU AC NC relay open (PSU off)"));
  }

  // false = closed (on the NC relay)
  Embedded().ShiftRegister(m_psuRelayPin, pv);
  Embedded().ShiftRegister(m_pvRelayPin, !pv);

  Embedded().ShiftRegister(m_psuLedPin, !pv);
  Embedded().ShiftRegister(m_batteryLedPin, pv);

  m_pvPowerSource = pv;
  Log().Trace(F("Source: %s"), pv ? "PV" : "PSU");
  Embedded().OnPowerSwitch();
}

void Power::RelayTest()
{
  SwitchPower(s_testState);
  s_testState = !s_testState;
  delay(1000);
}

float Power::ReadCommonVoltage()
{
  float f = Embedded().ReadCommonVoltageSensor();
  if (f == k_unknown) {
    return k_unknown;
  }
  return mapFloat(f, 0, k_commonVoltageMapIn, 0, k_commonVoltageMapOut);
}

float Power::ReadPsuVoltage()
{
  float f = Embedded().ReadPsuVoltageSensor();
  if (f == k_unknown) {
    return k_unknown;
  }
  return mapFloat(f, 0, k_psuVoltageMapIn, 0, k_psuVoltageMapOut);
}

// free function definitions

} // namespace greenhouse
} // namespace embedded