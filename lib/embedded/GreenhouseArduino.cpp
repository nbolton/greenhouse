#include "GreenhouseArduino.h"

#include <cstdarg>

#include "ho_config.h"

#include <Adafruit_SHT31.h>
#include <Arduino.h>
#include <BlynkSimpleEsp8266.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <MultiShiftRegister.h>
#include <NTPClient.h>
#include <OneWire.h>
#include <PCF8574.h>
#include <SPI.h>
#include <Thread.h>
#include <WiFiUdp.h>
#include <Wire.h>

struct ADC {
  String name;
  ADS1115_WE ads;
  bool ready = false;
};

// regular pins
const int k_oneWirePin = D3;
const int k_shiftRegisterLatchPin = D5; // RCLK (12)
const int k_shiftRegisterDataPin = D6;  // SER (14)
const int k_shiftRegisterClockPin = D7; // SRCLK (11)
const int k_actuatorPinA = D8;

// msr pins
const int k_relayPin = 0;
const int k_startBeepPin = 1;
const int k_switchPins[] = {0 + 8, 1 + 8, 2 + 8, 3 + 8};

// io1 pins
const int k_actuatorPin1 = P6;
const int k_actuatorPin2 = P7;

// adc1 pins
const ADS1115_MUX k_moisturePin = ADS1115_COMP_1_GND;

// adc2 pins
const ADS1115_MUX k_voltagePin = ADS1115_COMP_0_GND;
const ADS1115_MUX k_currentPin = ADS1115_COMP_1_GND;

const int k_shiftRegisterTotal = 2;
const int k_voltAverageCountMax = 100;
const int k_pvVoltageMin = 5; // V, in case of sudden voltage drop
const int k_fanSwitch = 0;
const int k_pumpSwitch1 = 1;
const int k_pumpSwitch2 = 2;
const int k_ledFlashDelay = 30; // ms
const int k_soilProbeIndex = 0;
const int k_waterProbeIndex = 1;
const byte k_tempSensorAddr1 = 0x44;
const byte k_tempSensorAddr2 = 0x45;

static PCF8574 s_io1(0x20);
static MultiShiftRegister s_shiftRegisters(
  k_shiftRegisterTotal, k_shiftRegisterLatchPin, k_shiftRegisterClockPin, k_shiftRegisterDataPin);
static Thread s_relayThread = Thread();
static OneWire s_oneWire(k_oneWirePin);
static DallasTemperature s_dallas(&s_oneWire);
static WiFiUDP s_ntpUdp;
static NTPClient s_timeClient(s_ntpUdp);
static char s_reportBuffer[80];
static BlynkTimer s_timer;
static Adafruit_SHT31 s_temperatureSensor1;
static Adafruit_SHT31 s_temperatureSensor2;
static ADC s_adc1, s_adc2;
static GreenhouseArduino *s_instance = nullptr;

GreenhouseArduino &GreenhouseArduino::Instance() { return *s_instance; }

void GreenhouseArduino::Instance(GreenhouseArduino &ga) { s_instance = &ga; }

void relayCallback() { s_instance->RelayCallback(); }
void refreshTimer() { s_instance->Refresh(); }

GreenhouseArduino::GreenhouseArduino() :
  m_log(),
  m_insideTemperature(k_unknown),
  m_insideHumidity(k_unknown),
  m_outsideTemperature(k_unknown),
  m_outsideHumidity(k_unknown),
  m_timerId(k_unknown),
  m_led(LOW),
  m_fakeInsideHumidity(k_unknown),
  m_fakeSoilTemperature(k_unknown),
  m_fakeSoilMoisture(k_unknown),
  m_refreshBusy(false),
  m_soilMoisture(k_unknown),
  m_insideHumidityWarningSent(false),
  m_soilMoistureWarningSent(false)
{
}

void GreenhouseArduino::Setup()
{
  Greenhouse::Setup();

  if (k_trace) {
    Serial.begin(9600);

    // wait for serial to connect before first trace
    delay(1000);
  }

  Log().Trace("\n\nStarting system: %s", BLYNK_DEVICE_NAME);

  pinMode(k_shiftRegisterLatchPin, OUTPUT);
  pinMode(k_shiftRegisterClockPin, OUTPUT);
  pinMode(k_shiftRegisterDataPin, OUTPUT);
  s_shiftRegisters.shift();

  StartBeep(1);

  Wire.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(k_actuatorPinA, OUTPUT);

  s_io1.pinMode(k_actuatorPin1, OUTPUT);
  s_io1.pinMode(k_actuatorPin2, OUTPUT);

  s_io1.digitalWrite(k_actuatorPin1, LOW);
  s_io1.digitalWrite(k_actuatorPin2, LOW);

  // 10ms seems to be the minimum switch time to stop the relay coil "buzzing"
  // when it has insufficient power.
  s_relayThread.onRun(relayCallback);
  s_relayThread.setInterval(10);

  s_dallas.begin();
  s_timeClient.begin();

  MeasureVoltage();
  InitSensors();

  Blynk.begin(k_auth, k_ssid, k_pass);

  Log().Trace("System ready");
  StartBeep(2);
}

void GreenhouseArduino::Loop()
{
  Greenhouse::Loop();

  s_timeClient.update();
  Blynk.run();
  s_timer.run();

  if (s_relayThread.shouldRun()) {
    s_relayThread.run();
  }
}

void GreenhouseArduino::SwitchWaterBattery(bool on)
{
  // if no state change, do nothing.
  if (m_waterBatteryOn == on) {
    return;
  }

  Blynk.virtualWrite(V55, on);

  m_waterBatteryOn = on;
  if (on) {
    SetSwitch(k_pumpSwitch1, true);
  }
  else {
    SetSwitch(k_pumpSwitch1, false);
  }
}

void GreenhouseArduino::SwitchHeatingSystem(bool on)
{
  // if no state change, do nothing.
  if (m_heatingSystemOn == on) {
    return;
  }
  
  Blynk.virtualWrite(V56, on);

  m_heatingSystemOn = on;
  if (on) {
    SetSwitch(k_fanSwitch, true);

    // HACK: wait for fan to spool up. otherwise, this drains the caps and
    // causes the microcontroller to lose power. perhaps this can be fixed
    // by having a cap for the microcontroller and a diode to prevent the
    // power components from stealing the power.
    delay(5000);

    SetSwitch(k_pumpSwitch2, true);
  }
  else {
    SetSwitch(k_fanSwitch, false);
    SetSwitch(k_pumpSwitch2, false);
  }
}

void GreenhouseArduino::InitSensors()
{
  if (!s_temperatureSensor1.begin(k_tempSensorAddr1)) {
    Log().Trace("Couldn't find SHT31 #1");
  }

  if (!s_temperatureSensor2.begin(k_tempSensorAddr2)) {
    Log().Trace("Couldn't find SHT31 #2");
  }

  s_adc1.name = "ADS1115 #1";
  s_adc2.name = "ADS1115 #2";
  s_adc1.ads = ADS1115_WE(0x48);
  s_adc2.ads = ADS1115_WE(0x49);
  s_adc1.ready = s_adc1.ads.init();
  s_adc2.ready = s_adc2.ads.init();

  if (!s_adc1.ready) {
    Log().Trace("%s, not connected", s_adc1.name.c_str());
  }

  if (!s_adc2.ready) {
    Log().Trace("%s, not connected", s_adc2.name.c_str());
  }
}

bool GreenhouseArduino::Refresh()
{
  // TODO: use mutex lock?
  if (m_refreshBusy) {
    Log().Trace("Refresh busy, skipping");
    return false;
  }

  // TODO: this isn't an ideal mutex lock because the two threads could
  // hit this line at the same time.
  m_refreshBusy = true;
  bool ok = Greenhouse::Refresh();
  m_refreshBusy = false;

  MeasureCurrent();

  Blynk.virtualWrite(V28, m_pvPowerSource);
  Blynk.virtualWrite(V29, m_pvVoltageSensor);
  Blynk.virtualWrite(V30, m_pvVoltageOutput);
  Blynk.virtualWrite(V42, m_pvCurrentSensor);
  Blynk.virtualWrite(V43, m_pvCurrentOutput);

  return ok;
}

void GreenhouseArduino::FlashLed(LedFlashTimes times)
{
  // disable flashing LED until after the last write is done
  if (!m_lastWriteDone)
    return;

  for (int i = 0; i < (int)times * 2; i++) {
    m_led = (m_led == LOW) ? HIGH : LOW;
    digitalWrite(LED_BUILTIN, m_led);
    delay(k_ledFlashDelay);
  }
}

float GreenhouseArduino::InsideHumidity() const
{
  if (TestMode()) {
    return m_fakeInsideHumidity;
  }
  return m_insideHumidity;
}

float GreenhouseArduino::SoilTemperature() const
{
  if (TestMode()) {
    return m_fakeSoilTemperature;
  }
  return m_soilTemperature;
}

float GreenhouseArduino::SoilMoisture() const
{
  if (TestMode()) {
    return m_fakeSoilMoisture;
  }
  return m_soilMoisture;
}

bool GreenhouseArduino::ReadSensors()
{
  if (TestMode()) {
    return true;
  }

  int failures = 0;

  m_insideTemperature = s_temperatureSensor1.readTemperature();
  if (isnan(m_insideTemperature)) {
    m_insideTemperature = k_unknown;
    failures++;
  }

  m_insideHumidity = s_temperatureSensor1.readHumidity();
  if (isnan(m_insideHumidity)) {
    m_insideHumidity = k_unknown;
    failures++;
  }

  m_outsideTemperature = s_temperatureSensor2.readTemperature();
  if (isnan(m_outsideTemperature)) {
    m_outsideTemperature = k_unknown;
    failures++;
  }

  m_outsideHumidity = s_temperatureSensor2.readHumidity();
  if (isnan(m_outsideHumidity)) {
    m_outsideHumidity = k_unknown;
    failures++;
  }

  s_dallas.requestTemperatures();
  m_soilTemperature = s_dallas.getTempCByIndex(k_soilProbeIndex);
  if (isnan(m_soilTemperature)) {
    m_soilTemperature = k_unknown;
    failures++;
  }

  m_waterTemperature = s_dallas.getTempCByIndex(k_waterProbeIndex);
  if (isnan(m_waterTemperature)) {
    m_waterTemperature = k_unknown;
    failures++;
  }

  float moistureAnalog = ReadAdc(s_adc1, k_moisturePin);
  m_soilMoisture = CalculateMoisture(moistureAnalog);

  return failures == 0;
}

void GreenhouseArduino::SetWindowActuatorSpeed(int speed) { analogWrite(k_actuatorPinA, speed); }

void GreenhouseArduino::RunWindowActuator(bool forward)
{
  if (forward) {
    s_io1.digitalWrite(k_actuatorPin1, HIGH);
    s_io1.digitalWrite(k_actuatorPin2, LOW);
  }
  else {
    s_io1.digitalWrite(k_actuatorPin1, LOW);
    s_io1.digitalWrite(k_actuatorPin2, HIGH);
  }
}

void GreenhouseArduino::StopActuator()
{
  s_io1.digitalWrite(k_actuatorPin1, LOW);
  s_io1.digitalWrite(k_actuatorPin2, LOW);
}

void GreenhouseArduino::SystemDelay(unsigned long ms) { delay(ms); }

void GreenhouseArduino::Reset()
{
  FlashLed(k_ledReset);
  ReportWarning("System rebooting");

  Blynk.disconnect();
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {
  }
}

void GreenhouseArduino::StartBeep(int times)
{
  for (int i = 0; i < times; i++) {
    s_shiftRegisters.set_shift(k_startBeepPin);
    delay(100);

    s_shiftRegisters.clear_shift(k_startBeepPin);
    delay(200);
  }
}

void GreenhouseArduino::SetSwitch(int index, bool on)
{
  int pin = k_switchPins[index];

  if (on) {
    Log().Trace("SR pin set: %d", pin);

    s_shiftRegisters.set_shift(pin);
    m_switchState[index] = true;
  }
  else {
    Log().Trace("SR pin clear: %d", pin);

    s_shiftRegisters.clear_shift(pin);
    m_switchState[index] = false;
  }

  String switchStates;
  for (int i = 0; i < k_switchCount; i++) {
    switchStates += "S";
    switchStates += i;
    switchStates += "=";
    switchStates += m_switchState[i] ? "On" : "Off";
    if (i != (k_switchCount - 1)) {
      switchStates += ", ";
    }
  }
  Blynk.virtualWrite(V33, switchStates);
}

void GreenhouseArduino::ToggleActiveSwitch()
{
  if (!m_switchState[m_activeSwitch]) {
    Log().Trace("Toggle switch on: %d", m_activeSwitch);
    SetSwitch(m_activeSwitch, true);
  }
  else {
    Log().Trace("Toggle switch off: %d", m_activeSwitch);
    SetSwitch(m_activeSwitch, false);
  }
}

void GreenhouseArduino::RelayCallback()
{
  MeasureVoltage();

  if (m_pvForceOn) {
    if (!m_pvPowerSource) {
      SwitchPower(true);
    }
  }
  else if (m_pvPowerSource && (m_pvVoltageOutput <= k_pvVoltageMin)) {
    Log().Trace("PV voltage drop detected");
    SwitchPower(false);
    m_pvVoltageAverage = k_unknown;
  }
  else if (m_pvVoltageAverage != k_unknown) {

    if (
      !m_pvPowerSource && (m_pvVoltageSwitchOn != k_unknown) &&
      (m_pvVoltageAverage >= m_pvVoltageSwitchOn)) {
      SwitchPower(true);
    }
    else if (
      m_pvPowerSource && (m_pvVoltageSwitchOff != k_unknown) &&
      (m_pvVoltageAverage <= m_pvVoltageSwitchOff)) {
      SwitchPower(false);
    }
  }
}

float GreenhouseArduino::ReadAdc(ADC &adc, ADS1115_MUX channel)
{
  if (!adc.ready) {
    Log().Trace("%s, not ready", adc.name.c_str());
    return k_unknown;
  }

  // HACK: this keeps getting reset to default (possibly due to a power issue
  // when the relay switches), so force the volt range every time we're about to read.
  s_adc1.ads.setVoltageRange_mV(ADS1115_RANGE_6144);
  s_adc2.ads.setVoltageRange_mV(ADS1115_RANGE_6144);

  adc.ads.setCompareChannels(channel);
  adc.ads.startSingleMeasurement();

  int times = 0;
  while (adc.ads.isBusy()) {
    delay(10);
    if (times++ > 10) {
      Log().Trace("%s, busy", adc.name.c_str());
      return k_unknown;
    }
  }

  return adc.ads.getResult_V(); // alternative: getResult_mV for Millivolt
}

void GreenhouseArduino::MeasureVoltage()
{
  if (!s_adc2.ready || m_pvVoltageSensorMax == k_unknown || m_pvVoltageOutputMax == k_unknown) {
    return;
  }

  m_pvVoltageSensor = ReadAdc(s_adc2, k_voltagePin);
  m_pvVoltageOutput = mapFloat(
    m_pvVoltageSensor,
    m_pvVoltageSensorMin,
    m_pvVoltageSensorMax,
    m_pvVoltageOutputMin,
    m_pvVoltageOutputMax);

  m_pvVoltageSum += m_pvVoltageOutput;
  m_pvVoltageCount++;

  if (m_pvVoltageCount >= k_voltAverageCountMax) {

    m_pvVoltageAverage = (m_pvVoltageSum / m_pvVoltageCount);

    m_pvVoltageCount = 0;
    m_pvVoltageSum = 0;
  }
}

void GreenhouseArduino::MeasureCurrent()
{
  if (!s_adc2.ready || m_pvCurrentSensorMax == k_unknown || m_pvCurrentOutputMax == k_unknown) {
    return;
  }

  m_pvCurrentSensor = ReadAdc(s_adc2, k_currentPin);
  m_pvCurrentOutput = mapFloat(
    m_pvCurrentSensor,
    m_pvCurrentSensorMin,
    m_pvCurrentSensorMax,
    m_pvCurrentOutputMin,
    m_pvCurrentOutputMax);
}

void GreenhouseArduino::SwitchPower(bool pv)
{
  Log().Trace("----");

  if (pv) {
    s_shiftRegisters.set_shift(k_relayPin);
  }
  else {
    s_shiftRegisters.clear_shift(k_relayPin);
  }

  Log().Trace(pv ? "PV on" : "PV off");
  m_pvPowerSource = pv;

  Log().Trace("Source: %s", pv ? "PV" : "PSU");

  Blynk.virtualWrite(V28, m_pvPowerSource);

  Log().Trace("----");
}

int GreenhouseArduino::CurrentHour() const { return s_timeClient.getHours(); }

void GreenhouseArduino::ReportInfo(const char *format, ...)
{
  FlashLed(k_ledSend);

  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  Blynk.logEvent("info", s_reportBuffer);
}

void GreenhouseArduino::ReportWarning(const char *format, ...)
{
  FlashLed(k_ledSend);

  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  Blynk.logEvent("warning", s_reportBuffer);
}

void GreenhouseArduino::ReportCritical(const char *format, ...)
{
  FlashLed(k_ledSend);

  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  Blynk.logEvent("critical", s_reportBuffer);
}

void GreenhouseArduino::ReportSensorValues()
{
  FlashLed(k_ledSend);
  Blynk.virtualWrite(V1, InsideTemperature());
  Blynk.virtualWrite(V2, InsideHumidity());
  Blynk.virtualWrite(V19, OutsideTemperature());
  Blynk.virtualWrite(V20, OutsideHumidity());
  Blynk.virtualWrite(V11, SoilTemperature());
  Blynk.virtualWrite(V21, SoilMoisture());
  Blynk.virtualWrite(V46, WaterTemperature());
}

void GreenhouseArduino::ReportWindowProgress()
{
  FlashLed(k_ledSend);
  Blynk.virtualWrite(V9, WindowProgress());
}

void GreenhouseArduino::ReportSystemInfo()
{
  FlashLed(k_ledSend);

  // current time
  Blynk.virtualWrite(V10, s_timeClient.getFormattedTime() + " UTC");

  // uptime
  int seconds, minutes, hours, days;
  long uptime = millis() / 1000;
  minutes = uptime / 60;
  seconds = uptime % 60;
  hours = minutes / 60;
  minutes = minutes % 60;
  days = hours / 24;
  hours = hours % 24;
  char uptimeBuffer[50];
  sprintf(uptimeBuffer, "%dd %dh %dm %ds", days, hours, minutes, seconds);
  Blynk.virtualWrite(V12, uptimeBuffer);

  // free heap
  int freeHeap = ESP.getFreeHeap();
  Log().Trace("Free heap: %d bytes", freeHeap);
  Blynk.virtualWrite(V13, (float)freeHeap / 1000);
}

void GreenhouseArduino::ReportWarnings()
{
  if (!m_soilMoistureWarningSent && (m_soilMoisture <= SoilMostureWarning())) {
    ReportWarning("Soil moisture low (%d%%)", m_soilMoisture);
    m_soilMoistureWarningSent = true;
  }

  if (!m_insideHumidityWarningSent && (m_insideHumidity >= InsideHumidityWarning())) {
    ReportWarning("Inside humidity high (%d%%)", m_insideHumidity);
    m_insideHumidityWarningSent = true;
  }
}

void GreenhouseArduino::HandleLastWrite()
{
  if (m_lastWriteDone) {
    return;
  }

  m_lastWriteDone = true;

  s_instance->TraceFlash(F("Handling last Blynk write"));

  // if this is the first time that the last write was done,
  // this means that the system has started.
  HandleSystemStarted();
}

void GreenhouseArduino::HandleSystemStarted()
{
  // run the first refresh (instead of waiting for the 1st refresh timer).
  // we run the 1st refresh here instead of when the timer is created,
  // because when we setup the timer for the first time, we may not
  // have all of the correct initial values.
  Refresh();

  FlashLed(k_ledStarted);
  Log().TraceFlash(F("System started"));
  ReportInfo("System started");
}

void GreenhouseArduino::HandleAutoMode(bool autoMode)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle auto mode: %s", (autoMode ? "Auto" : "Manual"));

  AutoMode(autoMode == 1);

  // light on for manual mode
  m_led = !autoMode ? LOW : HIGH;
  digitalWrite(LED_BUILTIN, m_led);
}

void GreenhouseArduino::HandleOpenStart(float openStart)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle open start temperature: %.2fC", openStart);

  OpenStart(openStart);
}

void GreenhouseArduino::HandleOpenFinish(float openFinish)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle open finish temperature: %.2fC", openFinish);

  OpenFinish(openFinish);
}

void GreenhouseArduino::HandleRefreshRate(int refreshRate)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle refresh rate: %ds", refreshRate);

  if (refreshRate <= 0) {
    Log().Trace("Invalid refresh rate: %ds", refreshRate);
    return;
  }

  if (m_timerId != k_unknown) {
    Log().Trace("Deleting old timer: %d", m_timerId);
    s_timer.deleteTimer(m_timerId);
  }

  m_timerId = s_timer.setInterval(refreshRate * 1000L, refreshTimer);
  Log().Trace("New refresh timer: %d", m_timerId);
}

void GreenhouseArduino::HandleWindowProgress(int windowProgress)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle window progress: %ds", windowProgress);

  if (WindowProgress() != k_unknown) {
    // only apply window progress if it's not the 1st time;
    // otherwise the window will always open from 0 on start,
    // and the position might be something else.
    ApplyWindowProgress((float)windowProgress / 100);
  }

  WindowProgress(windowProgress);
}

void GreenhouseArduino::HandleReset(int reset)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle reset: %d", reset);

  if (reset == 1) {
    Reset();
  }
}

void GreenhouseArduino::HandleRefresh(int refresh)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle refresh: %d", refresh);

  if (refresh == 1) {
    Refresh();
  }
}

void GreenhouseArduino::HandleFakeInsideHumidity(float fakeInsideHumidity)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle fake inside humidity: %.2f%%", fakeInsideHumidity);

  m_fakeInsideHumidity = fakeInsideHumidity;
}

void GreenhouseArduino::HandleFakeSoilTemperature(float fakeSoilTemperature)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle fake soil temperature: %.2fC", fakeSoilTemperature);

  m_fakeSoilTemperature = fakeSoilTemperature;
}

void GreenhouseArduino::HandleFakeSoilMoisture(float fakeSoilMoisture)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle fake soil moisture: %.2f%%", fakeSoilMoisture);

  m_fakeSoilMoisture = fakeSoilMoisture;
}

void GreenhouseArduino::HandleTestMode(int testMode)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle test mode: %s", testMode == 1 ? "On" : "Off");

  TestMode(testMode == 1);
}

void GreenhouseArduino::HandleOpenDayMinimum(int openDayMinimum)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle open day minimum: %d", openDayMinimum);

  OpenDayMinimum(openDayMinimum);
}

void GreenhouseArduino::HandleInsideHumidityWarning(float insideHumidityWarning)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle inside humidity warning: %d", insideHumidityWarning);

  InsideHumidityWarning(insideHumidityWarning);
}

void GreenhouseArduino::HandleSoilMostureWarning(float soilMostureWarning)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle soil moisture warning: %d", soilMostureWarning);

  SoilMostureWarning(soilMostureWarning);
}

void GreenhouseArduino::HandleActiveSwitch(int activeSwitch)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle toggle switch");

  m_activeSwitch = activeSwitch;
}

void GreenhouseArduino::HandleToggleSwitch(int toggleSwitch)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle toggle switch");

  if (toggleSwitch == 1) {
    ToggleActiveSwitch();
  }
}

BLYNK_CONNECTED()
{
  // read all last known values from Blynk server
  Blynk.syncVirtual(
    V0,
    V3,
    V5,
    V8,
    V9,
    V14,
    V15,
    V16,
    V17,
    V18,
    V22,
    V23,
    V24,
    V25,
    V26,
    V27,
    V31,
    V32,
    V33,
    V34,
    V35,
    V36,
    V37,
    V38,
    V39,
    V40,
    V41,
    V44,
    V45,
    V47,
    V48,
    V49,
    V50,
    V51,
    V53,
    V54);
}

BLYNK_WRITE(V0)
{
  s_instance->TraceFlash(F("Blynk write V0"));
  s_instance->HandleAutoMode(param.asInt());
}

BLYNK_WRITE(V3)
{
  s_instance->TraceFlash(F("Blynk write V3"));
  s_instance->HandleRefreshRate(param.asInt());
}

BLYNK_WRITE(V5)
{
  s_instance->TraceFlash(F("Blynk write V5"));
  s_instance->HandleOpenStart(param.asFloat());
}

BLYNK_WRITE(V6)
{
  s_instance->TraceFlash(F("Blynk write V6"));
  s_instance->HandleReset(param.asInt());
}

BLYNK_WRITE(V7)
{
  s_instance->TraceFlash(F("Blynk write V7"));
  s_instance->HandleRefresh(param.asInt());
}

BLYNK_WRITE(V8)
{
  s_instance->TraceFlash(F("Blynk write V8"));
  s_instance->HandleOpenFinish(param.asFloat());
}

BLYNK_WRITE(V9)
{
  s_instance->TraceFlash(F("Blynk write V9"));
  s_instance->HandleWindowProgress(param.asInt());
}

BLYNK_WRITE(V14)
{
  s_instance->TraceFlash(F("Blynk write V14"));
  s_instance->HandleTestMode(param.asInt());
}

BLYNK_WRITE(V15)
{
  s_instance->TraceFlash(F("Blynk write V15"));
  s_instance->HandleOpenDayMinimum(param.asInt());
}

BLYNK_WRITE(V16)
{
  s_instance->TraceFlash(F("Blynk write V16"));
  s_instance->HandleInsideHumidityWarning(param.asFloat());
}

BLYNK_WRITE(V17)
{
  s_instance->TraceFlash(F("Blynk write V17"));
  s_instance->HandleSoilMostureWarning(param.asFloat());
}

BLYNK_WRITE(V18)
{
  s_instance->TraceFlash(F("Blynk write V18"));
  s_instance->HandleFakeSoilTemperature(param.asFloat());
}

BLYNK_WRITE(V22)
{
  s_instance->TraceFlash(F("Blynk write V22"));
  s_instance->HandleFakeSoilMoisture(param.asFloat());
}

BLYNK_WRITE(V23)
{
  s_instance->TraceFlash(F("Blynk write V23"));
  s_instance->HandleFakeInsideHumidity(param.asFloat());
}

BLYNK_WRITE(V24)
{
  s_instance->TraceFlash(F("Blynk write V24"));
  s_instance->HandleActiveSwitch(param.asInt());
}

BLYNK_WRITE(V25)
{
  s_instance->TraceFlash(F("Blynk write V25"));
  s_instance->HandleToggleSwitch(param.asInt());
}

BLYNK_WRITE(V31)
{
  s_instance->TraceFlash(F("Blynk write V31"));
  s_instance->HandleDayStartHour(param.asInt());
}

BLYNK_WRITE(V32)
{
  s_instance->TraceFlash(F("Blynk write V32"));
  s_instance->HandleDayEndHour(param.asInt());
}

BLYNK_WRITE(V34)
{
  s_instance->TraceFlash(F("Blynk write V34"));
  s_instance->HandlePvVoltageSensorMin(param.asFloat());
}

BLYNK_WRITE(V35)
{
  s_instance->TraceFlash(F("Blynk write V35"));
  s_instance->HandlePvVoltageSensorMax(param.asFloat());
}

BLYNK_WRITE(V36)
{
  s_instance->TraceFlash(F("Blynk write V36"));
  s_instance->HandlePvVoltageOutputMin(param.asFloat());
}

BLYNK_WRITE(V37)
{
  s_instance->TraceFlash(F("Blynk write V37"));
  s_instance->HandlePvVoltageOutputMax(param.asFloat());
}

BLYNK_WRITE(V38)
{
  s_instance->TraceFlash(F("Blynk write V38"));
  s_instance->HandlePvCurrentSensorMin(param.asFloat());
}

BLYNK_WRITE(V39)
{
  s_instance->TraceFlash(F("Blynk write V39"));
  s_instance->HandlePvCurrentSensorMax(param.asFloat());
}

BLYNK_WRITE(V40)
{
  s_instance->TraceFlash(F("Blynk write V40"));
  s_instance->HandlePvCurrentOutputMin(param.asFloat());
}

BLYNK_WRITE(V41)
{
  s_instance->TraceFlash(F("Blynk write V41"));
  s_instance->HandlePvCurrentOutputMax(param.asFloat());
}

BLYNK_WRITE(V44)
{
  s_instance->TraceFlash(F("Blynk write V44"));
  s_instance->HandlePvVoltageSwitchOn(param.asFloat());
}

BLYNK_WRITE(V45)
{
  s_instance->TraceFlash(F("Blynk write V45"));
  s_instance->HandlePvVoltageSwitchOff(param.asFloat());
}

BLYNK_WRITE(V47)
{
  s_instance->TraceFlash(F("Blynk write V47"));
  s_instance->HandlePvForceOn(param.asInt());
}

BLYNK_WRITE(V48)
{
  s_instance->TraceFlash(F("Blynk write V48"));
  s_instance->HandleWindowOpenSpeed(param.asInt());
}

BLYNK_WRITE(V49)
{
  s_instance->TraceFlash(F("Blynk write V49"));
  s_instance->HandleWindowOpenRuntime(param.asFloat());
}

BLYNK_WRITE(V50)
{
  s_instance->TraceFlash(F("Blynk write V50"));
  s_instance->HandleDayWaterTemperature(param.asFloat());
}

BLYNK_WRITE(V51)
{
  s_instance->TraceFlash(F("Blynk write V51"));
  s_instance->HandleNightWaterTemperature(param.asFloat());
}

BLYNK_WRITE(V53)
{
  s_instance->TraceFlash(F("Blynk write V53"));
  s_instance->HandleDaySoilTemperature(param.asFloat());
}


BLYNK_WRITE(V54)
{
  s_instance->TraceFlash(F("Blynk write V54"));
  s_instance->HandleNightSoilTemperature(param.asFloat());

  // TODO: find a better way to always call this last; sometimes
  // when adding new write functions, moving this gets forgotten about.
  s_instance->HandleLastWrite();
}
