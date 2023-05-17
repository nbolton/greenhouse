#include "Power.h"

#include "common.h"

#include <Adafruit_INA219.h>
#include <Arduino.h>
#include <PCF8574.h>

using namespace common;

namespace greenhouse {
namespace embedded {

#define VS_PIN_PSU 32
#define VS_PIN_BATT 35
#define VS_PIN_PV 34
#define VS_IN_MIN 912
#define VS_IN_MAX 1575
#define VS_OUT_MIN 9.89
#define VS_OUT_MAX 15.87
#define PIN_LOCAL_VOLTAGE 34
#define VOLTAGE_DIFF_DELTA .1f
#define CURRENT_DIFF_DELTA .1f
#define SWITCH_LIMIT 10000 // 10s
#define IO_PIN_PSU_LED P2
#define IO_PIN_BATT_LED P3
#define IO_PIN_BATT_SW P4
#define IO_PIN_PSU_SW P5
#define IO_PIN_AC_SW P6
#define SWITCH_ON HIGH
#define SWITCH_OFF LOW
#define LED_ON LOW
#define LED_OFF HIGH
#define INA219_ADDR1 0x40
#define INA219_ADDR2 0x48

const float k_batteryVoltageMin = 10;
const float k_psuVoltageMin = 11.5;
const int k_switchDelay = 500; // delay between switching both relays

static Power *s_instance = nullptr;
bool s_testState = false;

Adafruit_INA219 ina219_0(INA219_ADDR1);
Adafruit_INA219 ina219_1(INA219_ADDR2);

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
  m_batteryCurrentSensor(k_unknown),
  m_batteryCurrentOutput(k_unknown),
  m_psuVoltageSensor(k_unknown),
  m_psuVoltageOutput(k_unknown),
  m_lastBatteryVoltage(k_unknown),
  m_lastBatteryCurrent(k_unknown),
  m_lastPsuVoltage(k_unknown),
  m_lastPsuCurrent(k_unknown),
  m_lastMeasure(k_unknown),
  m_nextSwitch(k_unknownUL)
{
}

void Power::Setup()
{
  s_instance = this;

  // circuit default state is both power sources connected
  Embedded().WriteOnboardIO(IO_PIN_PSU_LED, LED_ON);
  Embedded().WriteOnboardIO(IO_PIN_BATT_LED, LED_ON);

  ina219_0.begin();
  ina219_1.begin();
}

greenhouse::native::ISystem &Power::Native() const
{
  if (m_native == nullptr) {
    TRACE("Native system not set");
    throw;
  }
  return *m_native;
}

greenhouse::embedded::ISystem &Power::Embedded() const
{
  if (m_embedded == nullptr) {
    TRACE("Embedded system not set");
    throw;
  }
  return *m_embedded;
}

bool Power::batteryIsLow() const
{
  const bool vBattKnown = (m_batteryVoltageOutput != k_unknown);
  const bool vBattOffKnown = (m_batteryVoltageSwitchOff != k_unknown);
  return !vBattKnown || (vBattOffKnown && (m_batteryVoltageOutput <= m_batteryVoltageSwitchOff));
}

bool Power::batteryIsCharged() const
{
  const bool vBattKnown = (m_batteryVoltageOutput != k_unknown);
  const bool vBattOnKnown = (m_batteryVoltageSwitchOn != k_unknown);
  return vBattKnown && vBattOnKnown && (m_batteryVoltageOutput >= m_batteryVoltageSwitchOn);
}

void Power::Loop()
{
  MeasureVoltage();

  const float psuVoltage = m_psuVoltageOutput;
  const bool psuVoltageChanged = abs(m_lastPsuVoltage - psuVoltage) > VOLTAGE_DIFF_DELTA;
  if (psuVoltageChanged) {
    TRACE_F("PSU voltage changed, was %.2fV, now %.2fV", m_lastPsuVoltage, psuVoltage);

#if POWER_EN
    if ((m_source == PowerSource::k_powerSourcePsu) && (psuVoltage < k_psuVoltageMin)) {
      TRACE("PSU undervoltage, switching to battery");
      switchSource(PowerSource::k_powerSourceBattery);
      Native().ReportWarning("PSU undervoltage, using battery");
    }
#endif // POWER_EN
  }
  m_lastPsuVoltage = psuVoltage;

  const float batteryVoltage = m_batteryVoltageOutput;
  const bool batteryVoltageChanged =
    abs(m_lastBatteryVoltage - batteryVoltage) > VOLTAGE_DIFF_DELTA;
  if (batteryVoltageChanged) {
    TRACE_F("Battery voltage changed, was %.2fV now %.2fV", m_lastBatteryVoltage, batteryVoltage);

#if POWER_EN
    if ((m_source == PowerSource::k_powerSourceBattery) && (batteryVoltage < k_batteryVoltageMin)) {
      TRACE("Battery undervoltage, switching to PSU");
      switchSource(PowerSource::k_powerSourcePsu);
      Native().ReportWarning("Battery undervoltage, using PSU");
    }
#endif // POWER_EN
  }
  m_lastBatteryVoltage = m_batteryVoltageOutput;

#if POWER_EN
  if ((m_nextSwitch == k_unknownUL) || (millis() > m_nextSwitch)) {
    m_nextSwitch = millis() + SWITCH_LIMIT;
    if (Mode() != PowerMode::k_powerModeAuto) {
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
      if (batteryIsCharged()) {
        if (m_source != PowerSource::k_powerSourceBattery) {
          TRACE("Switching to battery automatically (PSU off)");
          switchSource(PowerSource::k_powerSourceBattery);
        }
      }
      else if (batteryIsLow()) {
        if (m_source != PowerSource::k_powerSourcePsu) {
          TRACE("Switching PSU on automatically (battery off)");
          switchSource(PowerSource::k_powerSourcePsu);
        }
      }
    }
  }
#endif // POWER_EN

  MeasureCurrent();

  if (abs(m_lastBatteryCurrent - BatteryCurrentOutput()) > CURRENT_DIFF_DELTA) {
    TRACE_F(
      "Battery current changed, was %.2fA, now %.2fA", //
      m_lastBatteryCurrent,
      BatteryCurrentOutput());
    Embedded().OnBatteryCurrentChange();
  }
  m_lastBatteryCurrent = BatteryCurrentOutput();
}

void Power::MeasureVoltage()
{
  m_batteryVoltageSensor = analogRead(VS_PIN_BATT);
  if (m_batteryVoltageSensor != k_unknown) {
    m_batteryVoltageOutput =
      mapFloat(m_batteryVoltageSensor, VS_IN_MIN, VS_IN_MAX, VS_OUT_MIN, VS_OUT_MAX);
  }
  else {
    m_batteryVoltageOutput = k_unknown;
  }

  m_psuVoltageSensor = analogRead(VS_PIN_PSU);
  if (m_psuVoltageSensor != k_unknown) {
    m_psuVoltageOutput = mapFloat(m_psuVoltageSensor, VS_IN_MIN, VS_IN_MAX, VS_OUT_MIN, VS_OUT_MAX);
  }
  else {
    m_psuVoltageOutput = k_unknown;
  }
}

// TODO: send the correct calibration value for a 0.001 ohms
// shunt (the Adafruit library uses a 0.1 ohms shunt).
float toAmps_0R001(float mA_0R1) { return mA_0R1 / 10; }

void Power::MeasureCurrent()
{
  Adafruit_INA219 &ina219 = ina219_0;
  float shunt = ina219.getShuntVoltage_mV();
  float bus = ina219.getBusVoltage_V();
  float current = toAmps_0R001(ina219.getCurrent_mA());
  float power = ina219.getPower_mW();
  float load = bus + (shunt / 1000);

  m_batteryCurrentOutput = current;
}

void Power::switchSource(PowerSource source)
{
  if (source == PowerSource::k_powerSourcePsu) {

    // ensure PSU AC is on and give it a moment to charge caps.
    TRACE("Switch PSU on");
    Embedded().WriteOnboardIO(IO_PIN_AC_SW, SWITCH_ON);
    Embedded().WriteOnboardIO(IO_PIN_PSU_LED, LED_ON);
    Embedded().Delay(k_switchDelay, "PSU AC relay");

    MeasureVoltage();
    if (m_psuVoltageOutput < k_psuVoltageMin) {
      TRACE_F("Unable to use PSU, voltage too low: %.2fV", m_psuVoltageOutput);
      Embedded().WriteOnboardIO(IO_PIN_AC_SW, SWITCH_OFF);
      Embedded().WriteOnboardIO(IO_PIN_PSU_LED, LED_OFF);
      Native().ReportWarning("Can't switch, PSU undervoltage");
      return;
    }

    TRACE("PSU AC relay live (PSU on)");
    TRACE("Power source: PSU");
  }
  else if (source == PowerSource::k_powerSourceBattery) {
    if (batteryIsLow()) {
      TRACE_F("Unable to use battery, voltage too low: %.2fV", m_batteryVoltageOutput);
      Native().ReportWarning("Can't switch, low battery");
      return;
    }

    TRACE("Switch battery on");
    Embedded().WriteOnboardIO(IO_PIN_AC_SW, SWITCH_OFF);
  }
  else {
    TRACE("Invalid power switch");
    return;
  }

  Embedded().WriteOnboardIO(IO_PIN_PSU_SW, source == k_powerSourcePsu ? SWITCH_ON : SWITCH_OFF);
  Embedded().WriteOnboardIO(
    IO_PIN_BATT_SW, source == k_powerSourceBattery ? SWITCH_ON : SWITCH_OFF);

  Embedded().WriteOnboardIO(IO_PIN_PSU_LED, source == k_powerSourcePsu ? LED_ON : LED_OFF);
  Embedded().WriteOnboardIO(IO_PIN_BATT_LED, source == k_powerSourceBattery ? LED_ON : LED_OFF);

  m_source = source;

  Embedded().OnPowerSwitch();
}

// free function definitions

} // namespace embedded
} // namespace greenhouse