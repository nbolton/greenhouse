#include "Power.h"

#include "common.h"

#include <Thread.h>

using namespace common;

namespace embedded {
namespace greenhouse {

const int s_threadInterval = 10000;  // 10s
const int k_pvOnboardVoltageMin = 7; // V, in case of sudden voltage drop
const int k_pvOnboardVoltageMapIn = 745;
const float k_pvOnboardVoltageMapOut = 13.02;

static Power *s_instance = nullptr;
static Thread s_powerThread = Thread();

// free-functions declarations

void threadCallback() { s_instance->ThreadCallback(); }

// member functions

Power::Power(int psuRelayPin, int pvRelayPin) :
  m_embedded(nullptr),
  m_native(nullptr),
  m_log(),
  m_psuRelayPin(psuRelayPin),
  m_pvRelayPin(pvRelayPin),
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
}

void Power::Loop()
{
  if (s_powerThread.shouldRun()) {
    s_powerThread.run();
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
  float onboardVoltage = readPvOnboardVoltage();
  Log().Trace(
    F("Init power source, onboard=%.2fV, sensor=%.2fV, min=%.2fV"),
    onboardVoltage,
    m_pvVoltageOutput,
    sensorMin);

  if (
    (m_pvVoltageSwitchOn != k_unknown) && (m_pvVoltageOutput >= sensorMin) &&
    (onboardVoltage > k_pvOnboardVoltageMin)) {

    Log().Trace(F("Using PV on start"));
    SwitchPower(true);
  }
  else {
    Log().Trace(F("Using PSU on start"));
    SwitchPower(false);
  }
}

void Power::ThreadCallback()
{
  MeasureVoltage();
  float onboardVoltage = readPvOnboardVoltage();

  if (onboardVoltage <= k_pvOnboardVoltageMin) {
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
  if (!pv) {
    // close the PSU relay and give the PSU time to power up
    Embedded().ExpanderWrite(m_psuRelayPin, LOW);
    Log().Trace(F("PSU AC relay closed"));
    Embedded().Delay(1000);
  }

  Embedded().ShiftRegister(m_pvRelayPin, !pv);

  if (pv) {
    // open the PSU relay to turn off mains power when on PV
    Embedded().ExpanderWrite(m_psuRelayPin, HIGH);
    Log().Trace(F("PSU AC relay open"));
  }

  m_pvPowerSource = pv;
  Log().Trace(F("Source: %s"), pv ? "PV" : "PSU");
  Embedded().OnPowerSwitch();
}

// free function definitions

float readPvOnboardVoltage()
{
  // reads the onboard PV voltage (as opposed to measuring at the battery).
  // this is after any potential breaks in the circuit (eg if the battery
  // is disconnected or if the battery case switch is off).
  int analogValue = analogRead(A0);
  return mapFloat(analogValue, 0, k_pvOnboardVoltageMapIn, 0, k_pvOnboardVoltageMapOut);
}

} // namespace greenhouse
} // namespace embedded