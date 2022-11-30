#include "Power.h"

#include "common.h"

#include <Arduino.h>
#include <PCF8574.h>

using namespace common;

namespace embedded {
namespace greenhouse {

#define PIN_LOCAL_VOLTAGE 34
#define VOLTAGE_DIFF_DELTA .1f
#define VOLTAGE_DROP_WAIT 10000 // 10s

#define IO_PIN_BATT_SW P0
#define IO_PIN_PSU_SW P1
#define IO_PIN_BATT_LED P4
#define IO_PIN_PSU_LED P5

#define SWITCH_ON HIGH
#define SWITCH_OFF LOW
#define LED_ON LOW
#define LED_OFF HIGH

const float k_localVoltageMin = 9.5;
const float k_localVoltageMapIn = 1880;
const float k_localVoltageMapOut = 10.0;
const int k_switchDelay = 2000; // delay between switching both relays
const bool k_relayTest = false; // useful for testing power drop
const int k_relayTestInterval = 2000;

static Power *s_instance = nullptr;
bool s_testState = false;

// member functions

Power::Power() :
  m_embedded(nullptr),
  m_native(nullptr),
  m_mode(PowerMode::k_powerModeUnknown),
  m_source(PowerSource::k_powerSourceBoth),
  m_batteryVoltageSwitchOn(k_unknown),
  m_batteryVoltageSwitchOff(k_unknown),
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
  m_lastBatteryVoltage(k_unknown),
  m_nextVoltageDropSwitch(k_unknownUL)
{
}

void Power::Setup()
{
  s_instance = this;

  // circuit default state is both power sources connected
  Embedded().WriteIO(IO_PIN_PSU_LED, LED_ON);
  Embedded().WriteIO(IO_PIN_BATT_LED, LED_ON);
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

void Power::Loop()
{
  MeasureVoltage();

  const float localVoltage = ReadLocalVoltage();
  const bool localVoltageChanged = abs(m_lastLocalVoltage - localVoltage) > VOLTAGE_DIFF_DELTA;
  if (localVoltageChanged) {
    TRACE_F("Local voltage changed: %.2fV", localVoltage);
  }
  m_lastLocalVoltage = localVoltage;

  const float batteryVoltage = m_batteryVoltageOutput;
  const bool batteryVoltageChanged =
    abs(m_lastBatteryVoltage - batteryVoltage) > VOLTAGE_DIFF_DELTA;
  if (batteryVoltageChanged) {
    TRACE_F("Battery voltage changed: %.2fV", batteryVoltage);
  }
  m_lastBatteryVoltage = m_batteryVoltageOutput;

  if ((m_nextVoltageDropSwitch == k_unknownUL) || (millis() > m_nextVoltageDropSwitch)) {
    if (localVoltage <= k_localVoltageMin) {
      Native().ReportWarning("Local voltage drop detected: %.2fV", localVoltage);
      if (m_source == PowerSource::k_powerSourceBattery) {
        TRACE("Auto switching from battery to PSU");
        switchSource(PowerSource::k_powerSourcePsu);
      }
      else if (m_source == PowerSource::k_powerSourcePsu) {
        TRACE("Auto switching from PSU to battery");
        switchSource(PowerSource::k_powerSourceBattery);
      }
      else {
        TRACE("Not auto switching; other source");
      }
      m_nextVoltageDropSwitch = millis() + VOLTAGE_DROP_WAIT;
    }
    else if (Mode() != PowerMode::k_powerModeAuto) {
      if (Mode() == PowerMode::k_powerModeManualBattery) {
        if (m_source != PowerSource::k_powerSourceBattery) {
          Native().ReportWarning("Power mode is manual (battery)");
          switchSource(PowerSource::k_powerSourceBattery);
        }
      }
      else if (Mode() == PowerMode::k_powerModeManualPsu) {
        if (m_source != PowerSource::k_powerSourcePsu) {
          Native().ReportWarning("Power mode is manual (PSU)");
          switchSource(PowerSource::k_powerSourcePsu);
        }
      }
    }
    else {
      const bool vBattKnown = (m_batteryVoltageOutput != k_unknown);
      const bool vBattOnKnown = (m_batteryVoltageSwitchOn != k_unknown);
      const bool vBattOffKnown = (m_batteryVoltageSwitchOff != k_unknown);

      const bool switchToBattery =
        vBattKnown && vBattOnKnown && (m_batteryVoltageOutput >= m_batteryVoltageSwitchOn);

      const bool switchToPsu =
        !vBattKnown || (vBattOffKnown && (m_batteryVoltageOutput <= m_batteryVoltageSwitchOff));

      if (switchToBattery) {
        if (m_source != PowerSource::k_powerSourceBattery) {
          TRACE("Switching to battery automatically (PSU off)");
          switchSource(PowerSource::k_powerSourceBattery);
        }
      }
      else if (switchToPsu) {
        if (m_source != PowerSource::k_powerSourcePsu) {
          TRACE("Switching PSU on automatically (battery off)");
          switchSource(PowerSource::k_powerSourcePsu);
        }
      }
    }
  }
}

void Power::MeasureVoltage()
{
  if (m_batteryVoltageSensorMax == k_unknown || m_batteryVoltageOutputMax == k_unknown) {
    return;
  }

  m_batteryVoltageSensor = Embedded().ReadBatteryVoltageSensorRaw();
#if TRACE_SENSOR_RAW
  TRACE_F("Battery voltage sensor raw: %.4f", m_batteryVoltageSensor);
#endif // TRACE_SENSOR_RAW
  if (m_batteryVoltageSensor != k_unknown) {
    m_batteryVoltageOutput = mapFloat(
      m_batteryVoltageSensor,
      m_batteryVoltageSensorMin,
      m_batteryVoltageSensorMax,
      m_batteryVoltageOutputMin,
      m_batteryVoltageOutputMax);
  }
  else {
    m_batteryVoltageOutput = k_unknown;
  }
}

void Power::MeasureCurrent()
{
  if (
    !Embedded().BatterySensorReady() || m_batteryCurrentSensorMax == k_unknown ||
    m_batteryCurrentOutputMax == k_unknown) {
    return;
  }

  m_batteryCurrentSensor = Embedded().ReadBatteryCurrentSensorRaw();
#if TRACE_SENSOR_RAW
  TRACE_F("Battery current sensor raw: %.4f", m_batteryCurrentSensor);
#endif // TRACE_SENSOR_RAW
  m_batteryCurrentOutput = mapFloat(
    m_batteryCurrentSensor,
    m_batteryCurrentSensorMin,
    m_batteryCurrentSensorMax,
    m_batteryCurrentOutputMin,
    m_batteryCurrentOutputMax);
}

void Power::switchSource(PowerSource source)
{
  m_source = source;

  // first, ensure constant power; close PSU NC relay and turn on battery FET
  Embedded().WriteIO(IO_PIN_PSU_SW, SWITCH_ON);
  Embedded().WriteIO(IO_PIN_BATT_SW, SWITCH_ON);
  Embedded().WriteIO(IO_PIN_PSU_LED, LED_ON);
  Embedded().WriteIO(IO_PIN_BATT_LED, LED_ON);

  // if on battery, give the PSU time to power up, or,
  // if on PSU, allow the battery relay to close first.
  Embedded().Delay(k_switchDelay, "Relay");

  if (source == PowerSource::k_powerSourcePsu) {
    TRACE("PSU AC NC relay closed (PSU on), battery NC relay open (battery off)");
    TRACE("Power source: PSU");
  }
  else if (source == PowerSource::k_powerSourceBattery) {
    TRACE("Battery NC relay closed (battery on), PSU AC NC relay open (PSU off)");
    TRACE("Power source: Battery");
  }
  else {
    TRACE("Battery NC relay closed (battery on), PSU AC NC relay closed (PSU on)");
    TRACE("Power source: Both (PSU and battery)");
  }

  // both relays are NC (normally closed), so to disconnect a source,
  // pass true which will open the relay.
  Embedded().WriteIO(IO_PIN_PSU_SW, m_source == k_powerSourcePsu ? SWITCH_ON : SWITCH_OFF);
  Embedded().WriteIO(IO_PIN_BATT_SW, m_source == k_powerSourceBattery ? SWITCH_ON : SWITCH_OFF);

  Embedded().WriteIO(IO_PIN_PSU_LED, m_source == k_powerSourcePsu ? LED_ON : LED_OFF);
  Embedded().WriteIO(IO_PIN_BATT_LED, m_source == k_powerSourceBattery ? LED_ON : LED_OFF);

  TRACE_F("Local voltage: %.2fV", ReadLocalVoltage());
  Embedded().OnPowerSwitch();
}

float Power::ReadLocalVoltage()
{
  float f = analogReadMilliVolts(PIN_LOCAL_VOLTAGE);
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