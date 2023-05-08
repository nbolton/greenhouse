#include "power.h"

#include <Arduino.h>

#include "common.h"
#include "io.h"
#include "trace.h"

#define VS_PIN_PSU 32
#define VS_PIN_BATT 35
#define VS_PIN_PV 34

#define VS_IN_MIN 912
#define VS_IN_MAX 1575
#define VS_OUT_MIN 9.89
#define VS_OUT_MAX 15.87
#define BATT_VOLTS_ON 12.5
#define BATT_VOLTS_OFF 11.5
#define AC_EN_DELAY 500
#define INIT_DELAY 20
#define SWITCH_RELAY
#define PSU_VOLTS_MIN 11

#define INA219_ADDR1 0x40
#define INA219_ADDR2 0x48

#define MODE i == 0 ? FCSTR("common") : FCSTR("pv")

Adafruit_INA219 ina219_0(INA219_ADDR1);
Adafruit_INA219 ina219_1(INA219_ADDR2);

namespace power {

enum source { kSourceBoth, kSourceBattery, kSourcePsu };

int i = 0;
float psuVolts = 0;
float battVolts = 0;
float pvVolts = 0;
int psuSensor;
int battSensor;
int pvSensor;
source activeSource = kSourceBoth;

bool initSwitch();
bool updateSwitch();

bool init()
{
  bool ok = true;

  // give the ADC a moment to measure correctly
  delay(INIT_DELAY);
  measureVoltage();
  printVoltage();

  // only sets values (does not begin io yet)
  if (!io::init()) {
    ok = false;
  }

  if (!initSwitch()) {
    ok = false;
  }

  // begin io after measuring, as not to affect measurement (e.g. if psu ac relay goes live).
  // begin after switch so that relay doesn't switch unneccesarily.
  if (!io::begin()) {
    return false;
  }

  if (!init(ina219_0, 0)) {
    ok = false;
  }

  if (!init(ina219_1, 1)) {
    ok = false;
  }

  return ok;
}

bool update()
{
  bool prompt = false;

  measureVoltage();

  if (updateSwitch()) {
    prompt = true;
  }

  return prompt;
}

bool initSwitch()
{
  if (battVolts > BATT_VOLTS_ON) {
    switchBattery();
  }
  else if (psuVolts > PSU_VOLTS_MIN) {
    switchPsu(false);
  }

  return true;
}

bool updateSwitch()
{
  switch (activeSource) {

  case kSourceBattery: {
    if (battVolts < BATT_VOLTS_OFF) {
      Serial.println();
      TRACE(TRACE_INFO, "low battery, switching to psu");
      switchPsu(true);
      return true;
    }
    break;
  }

  case kSourcePsu: {
    if (psuVolts < PSU_VOLTS_MIN) {
      Serial.println();
      TRACE(TRACE_ERROR, "psu undervoltage, switching to battery");
      switchBattery();
      return true;
    }
    break;
  }
  }

  return false;
}

bool init(Adafruit_INA219 &ina219, int i)
{
  TRACE_VA(TRACE_INFO, "init ina219 #%d (%s)", i, MODE);

  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A). However
  // you can call a setCalibration function to change this range (see comments).
  if (!ina219.begin()) {
    TRACE_VA(TRACE_ERROR, "failed init ina219 #%d", i);
    return false;
  }

  // To use a slightly lower 32V, 1A range (higher precision on amps):
  // ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  // ina219.setCalibration_16V_400mA();

  return true;
}

void switchBattery()
{
  if (battVolts < BATT_VOLTS_OFF) {
    TRACE_VA(TRACE_ERROR, "battery voltage too low, %.2f", battVolts);
    shutdown();
  }

  PCF8574 &pcf8574 = io::device(IO_DEV);
  TRACE(TRACE_INFO, "switching to battery");

  io::set(IO_DEV, BATT_EN, HIGH);
  io::set(IO_DEV, PSU_EN, LOW);

  io::set(IO_DEV, FAN_EN, LOW);
  io::set(IO_DEV, PSU_LED, HIGH);
  io::set(IO_DEV, BATT_LED, LOW);

#ifdef SWITCH_RELAY
  TRACE(TRACE_INFO, "ac relay disconnect");
  io::set(IO_DEV, AC_EN, LOW);
#endif // SWITCH_RELAY

  activeSource = kSourceBattery;
  TRACE(TRACE_INFO, "battery is source");
}

void switchPsu(bool warmup)
{
  PCF8574 &pcf8574 = io::device(IO_DEV);
  TRACE_VA(TRACE_INFO, "switching to psu, warmup=%d", warmup);

#ifdef SWITCH_RELAY
  TRACE(TRACE_INFO, "ac relay live");
  if (warmup) {
    TRACE(TRACE_DEBUG1, "psu warming up");
    io::set(IO_DEV, AC_EN, HIGH);
    delay(AC_EN_DELAY);
    measureVoltage();
  }
#endif // SWITCH_RELAY

  if (psuVolts < PSU_VOLTS_MIN) {
    TRACE_VA(TRACE_ERROR, "psu undervoltage, %.2f", psuVolts);
    shutdown();
  }

  io::set(IO_DEV, PSU_EN, HIGH);
  io::set(IO_DEV, BATT_EN, LOW);

  io::set(IO_DEV, FAN_EN, HIGH);
  io::set(IO_DEV, BEEP_EN, HIGH);
  io::set(IO_DEV, PSU_LED, LOW);
  io::set(IO_DEV, BATT_LED, HIGH);

  activeSource = kSourcePsu;
  TRACE(TRACE_INFO, "psu is source");
}

void toggleSource()
{
  if (activeSource == kSourceBattery) {
    switchPsu(true);
  }
  else {
    switchBattery();
  }
}

void measureVoltage()
{
  psuSensor = analogRead(VS_PIN_PSU);
  battSensor = analogRead(VS_PIN_BATT);
  pvSensor = analogRead(VS_PIN_PV);

  psuVolts = mapFloat(psuSensor, VS_IN_MIN, VS_IN_MAX, VS_OUT_MIN, VS_OUT_MAX);
  battVolts = mapFloat(battSensor, VS_IN_MIN, VS_IN_MAX, VS_OUT_MIN, VS_OUT_MAX);
  pvVolts = mapFloat(pvSensor, VS_IN_MIN, VS_IN_MAX, VS_OUT_MIN, VS_OUT_MAX);
}

void printVoltage()
{
  TRACE_VA(TRACE_DEBUG1, "sensors: psu=%d batt=%d pv=%d", psuSensor, battSensor, pvSensor);
  TRACE_VA(TRACE_INFO, "voltages: psu=%.2fV batt=%.2fV pv=%.2fV", psuVolts, battVolts, pvVolts);
}

// TODO: send the correct calibration value for a 0.001 ohms
// shunt (the Adafruit library uses a 0.1 ohms shunt).
float toAmps_0R001(float mA_0R1) { return mA_0R1 / 10; }

void printCurrent()
{
  measureCurrent(ina219_0, 0);
  measureCurrent(ina219_1, 1);
}

void measureCurrent(Adafruit_INA219 &ina219, int i)
{
  TRACE_VA(TRACE_INFO, "measure ina219 #%d (%s)", i, MODE);

  float shunt = ina219.getShuntVoltage_mV();
  float bus = ina219.getBusVoltage_V();
  float current = toAmps_0R001(ina219.getCurrent_mA());
  float power = ina219.getPower_mW();
  float load = bus + (shunt / 1000);

  TRACE_VA(TRACE_INFO, "bus: %.2f V", bus);
  TRACE_VA(TRACE_INFO, "shunt: %.2f mV", shunt);
  TRACE_VA(TRACE_INFO, "load: %.2f V", load);
  TRACE_VA(TRACE_INFO, "current: %.2f A", current);
  TRACE_VA(TRACE_INFO, "power: %.2f mW", power);
  TRACE(TRACE_INFO, "");
}

void shutdown()
{
  // set all i/o to default (HIGH)
  for (int d = 0; d < IO_DEVICES; d++) {
    for (int p = 0; p < IO_PORTS; p++) {
      io::device(d).digitalWrite(p, HIGH);
    }
  }

  HALT();
}

} // namespace power
