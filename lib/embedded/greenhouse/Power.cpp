#include "Power.h"

#include "common.h"

#include <Thread.h>

using namespace common;

namespace embedded {
namespace greenhouse {

const int s_threadInterval = 10000; // 10s
const int k_onboardVoltageMin = 7;  // V, in case of sudden voltage drop
const int k_pvOnboardVoltageMapIn = 745;
const float k_pvOnboardVoltageMapOut = 13.02;
const int k_switchDelay = 500; // delay between switching both relays
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

Power::Power(int psuRelayPin, int pvRelayPin, int batteryLedPin, int psuLedPin) :
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
    Log().Trace("Native system not set");
    throw;
  }
  return *m_native;
}

embedded::greenhouse::ISystem &Power::Embedded() const
{
  if (m_embedded == nullptr) {
    Log().Trace("Embedded system not set");
    throw;
  }
  return *m_embedded;
}

void Power::InitPowerSource()
{
  const float sensorMin = m_pvVoltageSwitchOff;

  MeasureVoltage();
  float onboardVoltage = readOnboardVoltage();
  Log().Trace(
    F("Init power source, onboard=%.2fV, sensor=%.2fV, min=%.2fV"),
    onboardVoltage,
    m_pvVoltageOutput,
    sensorMin);

  bool pvSensorAboveMin = m_pvVoltageOutput >= sensorMin;
  bool onboardAboveMin = onboardVoltage >= k_onboardVoltageMin;

  if ((m_pvVoltageSwitchOn != k_unknown) && pvSensorAboveMin && onboardAboveMin) {

    Log().Trace(F("Using PV on start (onboard voltage and PV sensor above min)"));
    SwitchPower(true);
  }
  else {

    if (!pvSensorAboveMin) {
      Log().Trace(F("PV sensor voltage is below min"));
    }

    if (!onboardAboveMin) {
      Log().Trace(F("PV onboard voltage is below min"));
    }

    Log().Trace(F("Using PSU on start"));
    SwitchPower(false);
  }
}

void Power::ThreadCallback()
{
  MeasureVoltage();
  float onboardVoltage = readOnboardVoltage();

  if (onboardVoltage <= k_onboardVoltageMin) {
    if (m_pvPowerSource) {
      Native().ReportWarning("PV voltage drop detected: %.2fV", onboardVoltage);
      SwitchPower(false);
    }
  }
  else if (PvMode() != PvModes::k_pvAuto) {
    if (!m_pvPowerSource && (PvMode() == PvModes::k_pvOn)) {
      SwitchPower(true);
    }
    else if (m_pvPowerSource && (PvMode() == PvModes::k_pvOff)) {
      SwitchPower(false);
    }
  }
  else if (m_pvVoltageOutput != k_unknown) {

    if (
      !m_pvPowerSource && (m_pvVoltageSwitchOn != k_unknown) &&
      (m_pvVoltageOutput >= m_pvVoltageSwitchOn)) {
      SwitchPower(true);
    }
    else if (
      m_pvPowerSource && (m_pvVoltageSwitchOff != k_unknown) &&
      (m_pvVoltageOutput <= m_pvVoltageSwitchOff)) {
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
    Log().Trace(F("PSU AC NC relay open (PSU off), PV NC relay closed (battery on)"));
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

// free function definitions

float readOnboardVoltage()
{
  // reads the onboard PV voltage (as opposed to measuring at the battery).
  // this is after any potential breaks in the circuit (eg if the battery
  // is disconnected or if the battery case switch is off).
  int analogValue = analogRead(A0);
  return mapFloat(analogValue, 0, k_pvOnboardVoltageMapIn, 0, k_pvOnboardVoltageMapOut);
}

} // namespace greenhouse
} // namespace embedded