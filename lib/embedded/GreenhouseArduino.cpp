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
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

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
const int k_pvOnboardVoltageMin = 7; // V, in case of sudden voltage drop
const int k_pvOnboardVoltageMapIn = 698;
const float k_pvOnboardVoltageMapOut = 12.1;
const int k_fanSwitch = 0;
const int k_pumpSwitch1 = 1;
const int k_pumpSwitch2 = 2;
const int k_ledFlashDelay = 30; // ms
const int k_soilProbeIndex = 0;
const int k_waterProbeIndex = 1;
const uint8_t k_insideAirSensorAddress = 0x44;
const uint8_t k_outsideAirSensorAddress = 0x45;
const int k_adcAddress1 = 0x48;
const int k_adcAddress2 = 0x49;
const float k_weatherLat = 54.203;
const float k_weatherLon = -4.408;
const char* k_weatherApiKey = "e8444a70abfc2b472d43537730750892";
const char* k_weatherHost = "api.openweathermap.org";
const char* k_weatherUri = "/data/2.5/weather?lat=%.3f&lon=%.3f&units=metric&appid=%s";
const int s_relayThreadInterval = 10; // 10 ms, high frequency in case of voltage drop
const uint8_t k_ioAddress = 0x20;

static PCF8574 s_io1(k_ioAddress);
static MultiShiftRegister s_shiftRegisters(
  k_shiftRegisterTotal, k_shiftRegisterLatchPin, k_shiftRegisterClockPin, k_shiftRegisterDataPin);
static Thread s_relayThread = Thread();
static OneWire s_oneWire(k_oneWirePin);
static DallasTemperature s_dallas(&s_oneWire);
static WiFiUDP s_ntpUdp;
static NTPClient s_timeClient(s_ntpUdp);
static char s_reportBuffer[80];
static BlynkTimer s_timer;
static Adafruit_SHT31 s_insideAirSensor;
static Adafruit_SHT31 s_outsideAirSensor;
static ADC s_adc1, s_adc2;
static GreenhouseArduino *s_instance = nullptr;
static WiFiClient s_wifiClient;

GreenhouseArduino &GreenhouseArduino::Instance() { return *s_instance; }

void GreenhouseArduino::Instance(GreenhouseArduino &ga) { s_instance = &ga; }

void relayCallback() { s_instance->RelayCallback(); }
void refreshTimer() { s_instance->Refresh(); }

GreenhouseArduino::GreenhouseArduino() :
  m_log(),
  m_insideAirTemperature(k_unknown),
  m_insideAirHumidity(k_unknown),
  m_outsideAirTemperature(k_unknown),
  m_outsideAirHumidity(k_unknown),
  m_soilTemperature(k_unknown),
  m_waterTemperature(k_unknown),
  m_timerId(k_unknown),
  m_led(LOW),
  m_fakeInsideHumidity(k_unknown),
  m_fakeSoilTemperature(k_unknown),
  m_fakeSoilMoisture(k_unknown),
  m_refreshBusy(false),
  m_lastWriteDone(false),
  m_soilMoisture(k_unknown),
  m_insideHumidityWarningSent(false),
  m_soilMoistureWarningSent(false),
  m_activeSwitch(k_unknown),
  m_pvPowerSource(false),
  m_pvVoltageSwitchOn(k_unknown),
  m_pvVoltageSwitchOff(k_unknown),
  m_pvVoltageCount(0),
  m_pvVoltageAverage(0),
  m_pvVoltageSum(0),
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
  m_pvForceOn(false)
{
  for (int i = 0; i < k_switchCount; i++) {
    m_switchState[i] = false;
  }
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

  InitShiftRegisters();

  StartBeep(1);

  Wire.begin();

  InitActuators();

  s_relayThread.onRun(relayCallback);
  s_relayThread.setInterval(s_relayThreadInterval);

  s_dallas.begin();
  s_timeClient.begin();

  MeasureVoltage();
  InitSensors();
  InitADCs();

  Blynk.begin(k_auth, k_ssid, k_pass);

  Log().Trace("System ready");
  Blynk.virtualWrite(V52, "Ready");
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

void GreenhouseArduino::InitShiftRegisters()
{
  pinMode(k_shiftRegisterLatchPin, OUTPUT);
  pinMode(k_shiftRegisterClockPin, OUTPUT);
  pinMode(k_shiftRegisterDataPin, OUTPUT);

  try {
    s_shiftRegisters.shift();
  }
  catch (...) {
    ReportCritical("Shift register init failed");
    throw;
  }
}

void GreenhouseArduino::InitActuators()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(k_actuatorPinA, OUTPUT);

  s_io1.pinMode(k_actuatorPin1, OUTPUT);
  s_io1.pinMode(k_actuatorPin2, OUTPUT);

  s_io1.digitalWrite(k_actuatorPin1, LOW);
  s_io1.digitalWrite(k_actuatorPin2, LOW);
}

void GreenhouseArduino::InitSensors()
{
  if (!s_insideAirSensor.begin(k_insideAirSensorAddress)) {
    ReportWarning("Inside air sensor not found");
  }

  if (!s_outsideAirSensor.begin(k_outsideAirSensorAddress)) {
    ReportWarning("Inside air sensor not found");
  }
}

void GreenhouseArduino::InitADCs()
{
  s_adc1.name = "ADS1115 #1";
  s_adc2.name = "ADS1115 #2";
  s_adc1.ads = ADS1115_WE(k_adcAddress1);
  s_adc2.ads = ADS1115_WE(k_adcAddress2);
  s_adc1.ready = s_adc1.ads.init();
  s_adc2.ready = s_adc2.ads.init();

  if (!s_adc1.ready) {
    ReportWarning("ADC not ready, init failed: %s", s_adc1.name.c_str());
  }

  if (!s_adc2.ready) {
    ReportWarning("ADC not ready, init failed: %s", s_adc2.name.c_str());
  }
}

void GreenhouseArduino::SwitchWaterHeating(bool on)
{
  if (on) {
    SetSwitch(k_pumpSwitch1, true);
  }
  else {
    SetSwitch(k_pumpSwitch1, false);
  }
}

void GreenhouseArduino::SwitchSoilHeating(bool on)
{
  if (on) {
    SetSwitch(k_pumpSwitch2, true);
  }
  else {
    // HACK: until the fan pump arrives, we're sharing the soil heating pump,
    // so only turn off if not in use
    if (!AirHeatingIsOn()) {
      SetSwitch(k_pumpSwitch2, false);
    }
  }
}

void GreenhouseArduino::SwitchAirHeating(bool on)
{
  if (on) {
    SetSwitch(k_fanSwitch, true);
    SetSwitch(k_pumpSwitch2, true);
  }
  else {
    SetSwitch(k_fanSwitch, false);

    // HACK: until the fan pump arrives, we're sharing the soil heating pump,
    // so only turn off if not in use
    if (!SoilHeatingIsOn()) {
      SetSwitch(k_pumpSwitch2, false);
    }
  }
}

bool GreenhouseArduino::Refresh()
{
  // TODO: use mutex lock?
  if (m_refreshBusy) {
    Log().Trace("Refresh busy, skipping");
    return false;
  }

  FlashLed(k_ledRefresh);

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
  Blynk.virtualWrite(V55, WaterHeatingIsOn());
  Blynk.virtualWrite(V56, SoilHeatingIsOn());
  Blynk.virtualWrite(V59, AirHeatingIsOn());

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

float GreenhouseArduino::InsideAirHumidity() const
{
  if (TestMode()) {
    return m_fakeInsideHumidity;
  }
  return m_insideAirHumidity;
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

bool GreenhouseArduino::ReadSensors(int& failures)
{
  if (TestMode()) {
    return true;
  }

  m_insideAirTemperature = s_insideAirSensor.readTemperature();
  if (isnan(m_insideAirTemperature)) {
    m_insideAirTemperature = k_unknown;
    failures++;
  }

  m_insideAirHumidity = s_insideAirSensor.readHumidity();
  if (isnan(m_insideAirHumidity)) {
    m_insideAirHumidity = k_unknown;
    failures++;
  }

  m_outsideAirTemperature = s_outsideAirSensor.readTemperature();
  if (isnan(m_outsideAirTemperature)) {
    m_outsideAirTemperature = k_unknown;
    failures++;
  }

  m_outsideAirHumidity = s_outsideAirSensor.readHumidity();
  if (isnan(m_outsideAirHumidity)) {
    m_outsideAirHumidity = k_unknown;
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
  if (moistureAnalog == k_unknown) {
    m_soilMoisture = k_unknown;
    failures++;
  }
  else {
    m_soilMoisture = CalculateMoisture(moistureAnalog);
    if (m_soilMoisture == k_unknown) {
      failures++;
    }
  }

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

void GreenhouseArduino::Restart()
{
  FlashLed(k_ledRestart);
  ReportWarning("System restarting");
  Blynk.virtualWrite(V52, "Restarting");

  for (int i = 0; i < k_switchCount; i++) {
    SetSwitch(i, false);
  }

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

    try {
      s_shiftRegisters.set_shift(pin);
    }
    catch (...) {
      ReportCritical("Switch on failed: %d", index);
      throw;
    }

    m_switchState[index] = true;
  }
  else {
    Log().Trace("SR pin clear: %d", pin);

    try {
      s_shiftRegisters.clear_shift(pin);
    }
    catch (...) {
      ReportCritical("Switch off failed: %d", index);
      throw;
    }

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

float readPvOnboardVoltage()
{
  // reads the onboard PV voltage (as opposed to measuring at the battery).
  // this is after any potential breaks in the circuit (eg if the battery
  // is disconnected or if the battery case switch is off).
  int analogValue = analogRead(A0);
  return mapFloat(analogValue, 0, k_pvOnboardVoltageMapIn, 0, k_pvOnboardVoltageMapOut);
}

void GreenhouseArduino::RelayCallback()
{
  MeasureVoltage();
  float onboardVoltage = readPvOnboardVoltage();

  if (onboardVoltage <= k_pvOnboardVoltageMin) {
    if (m_pvPowerSource) {
      ReportWarning("PV voltage drop detected: %.2fV", onboardVoltage);
      SwitchPower(false);
      m_pvVoltageAverage = k_unknown;
    }
  }
  else if (m_pvForceOn) {
    if (!m_pvPowerSource) {
      SwitchPower(true);
    }
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
    Log().Trace("ADC not ready: %s", adc.name.c_str());
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
      Log().Trace("ADC is busy: %s", adc.name.c_str());
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
  if (pv) {
    try {
      s_shiftRegisters.set_shift(k_relayPin);
    }
    catch (...) {
      ReportCritical("Switch power on failed");
      throw;
    }
  }
  else {
    try {
      s_shiftRegisters.clear_shift(k_relayPin);
    }
    catch (...) {
      ReportCritical("Switch power off failed");
      throw;
    }
  }

  m_pvPowerSource = pv;
  Log().Trace("Source: %s", pv ? "PV" : "PSU");
  Blynk.virtualWrite(V28, m_pvPowerSource);
}

int GreenhouseArduino::CurrentHour() const { return s_timeClient.getHours(); }

unsigned long GreenhouseArduino::UptimeSeconds() const { return millis() / 1000; }

void GreenhouseArduino::ReportInfo(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  Log().Trace(s_reportBuffer);
  Blynk.logEvent("info", s_reportBuffer);
}

void GreenhouseArduino::ReportWarning(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  Log().Trace(s_reportBuffer);
  Blynk.logEvent("warning", s_reportBuffer);
}

void GreenhouseArduino::ReportCritical(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  Log().Trace(s_reportBuffer);
  Blynk.logEvent("critical", s_reportBuffer);
}

void GreenhouseArduino::ReportSensorValues()
{
  FlashLed(k_ledSend);
  Blynk.virtualWrite(V1, InsideAirTemperature());
  Blynk.virtualWrite(V2, InsideAirHumidity());
  Blynk.virtualWrite(V19, OutsideAirTemperature());
  Blynk.virtualWrite(V20, OutsideAirHumidity());
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
  long uptime = UptimeSeconds();
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

  if (!m_insideHumidityWarningSent && (m_insideAirHumidity >= InsideHumidityWarning())) {
    ReportWarning("Inside humidity high (%d%%)", m_insideAirHumidity);
    m_insideHumidityWarningSent = true;
  }
}

void GreenhouseArduino::LastWrite()
{
  if (m_lastWriteDone) {
    return;
  }

  m_lastWriteDone = true;

  s_instance->TraceFlash(F("Handling last Blynk write"));

  // if this is the first time that the last write was done,
  // this means that the system has started.
  SystemStarted();
}

void GreenhouseArduino::SystemStarted()
{
  // loop() will not have run yet, so make sure the time is updated
  // before running the refresh function.
  s_timeClient.update();

  // run the first refresh (instead of waiting for the 1st refresh timer).
  // we run the 1st refresh here instead of when the timer is created,
  // because when we setup the timer for the first time, we may not
  // have all of the correct initial values.
  Refresh();

  FlashLed(k_ledStarted);
  ReportInfo("System started");
}

void GreenhouseArduino::RefreshRate(int refreshRate)
{
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

void GreenhouseArduino::HandleWindowProgress(int value)
{
  if (WindowProgress() != k_unknown) {
    // only apply window progress if it's not the 1st time;
    // otherwise the window will always open from 0 on start,
    // and the position might be something else.
    ApplyWindowProgress((float)value / 100);
  }

  WindowProgress(value);
}

char s_weatherInfo[50];
DynamicJsonDocument s_jsonDoc(1024);

void GreenhouseArduino::UpdateWeatherForecast()
{
  char uri[100];
  sprintf(uri, k_weatherUri, k_weatherLat, k_weatherLon, k_weatherApiKey);
  
  Log().Trace("Connecting to weather host: %s", k_weatherHost);
  HttpClient httpClient(s_wifiClient, k_weatherHost, 80);

  Log().Trace("Weather host get: %s", uri);
  httpClient.get(uri);
  
  int statusCode = httpClient.responseStatusCode();
  Log().Trace("Weather host status: %d", statusCode);

  if (statusCode != 200) {
    ReportWarning("Weather host error: %d", statusCode);
    return;
  }

  String response = httpClient.responseBody();
  Log().Trace("Weather host response length: %d", strlen(response.c_str()));

  DynamicJsonDocument& jsonDoc = s_jsonDoc;

  DeserializationError error = deserializeJson(jsonDoc, response);
  if (error != DeserializationError::Ok) {
    ReportWarning("Weather data error: %s", error.c_str());
    return;
  }

  Log().Trace("Deserialized weather JSON");
  int id = jsonDoc["weather"][0]["id"];
  const char* main = jsonDoc["weather"][0]["main"];
  int dt = jsonDoc["dt"];
  const char* location = jsonDoc["name"];

  int hours = (int)((dt % 86400L) / 3600);
  int minutes = (int)((dt % 3600) / 60);

  String hoursString = hours < 10 ? "0" + String(hours) : String(hours);
  String minuteString = minutes < 10 ? "0" + String(minutes) : String(minutes);

  sprintf(s_weatherInfo, "%s @ %s:%s UTC (%s)", main, hoursString.c_str(), minuteString.c_str(), location);

  WeatherCode(id);
  WeatherInfo(s_weatherInfo);

  Log().Trace("Weather forecast: code=%d, info='%s'", WeatherCode(), WeatherInfo().c_str());

  ReportWeather();
}

void GreenhouseArduino::ReportWeather()
{
  Blynk.virtualWrite(V60, WeatherInfo().c_str());
}

void GreenhouseArduino::ReportWaterHeatingRuntime()
{
  Blynk.virtualWrite(V62, WaterHeatingRuntimeSeconds());
}

void GreenhouseArduino::ManualRefresh()
{
  Log().TraceFlash(F("Manual refresh"));
  s_timeClient.update();
  Refresh();
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
    V54,
    V57,
    V58,
    V61,
    V62);
}

BLYNK_WRITE(V0)
{
  s_instance->AutoMode(param.asInt() == 1);
}

BLYNK_WRITE(V3)
{
  s_instance->RefreshRate(param.asInt());
}

BLYNK_WRITE(V5)
{
  s_instance->OpenStart(param.asFloat());
}

BLYNK_WRITE(V6)
{
  if (param.asInt() == 1) {
    s_instance->Restart();
  }
}

BLYNK_WRITE(V7)
{
  if (param.asInt() == 1) {
    s_instance->ManualRefresh();
  }
}

BLYNK_WRITE(V8)
{
  s_instance->OpenFinish(param.asFloat());
}

BLYNK_WRITE(V9)
{
  s_instance->HandleWindowProgress(param.asInt());
}

BLYNK_WRITE(V14)
{
  s_instance->TestMode(param.asInt() == 1);
}

BLYNK_WRITE(V15)
{
  s_instance->OpenDayMinimum(param.asInt());
}

BLYNK_WRITE(V16)
{
  s_instance->InsideHumidityWarning(param.asFloat());
}

BLYNK_WRITE(V17)
{
  s_instance->SoilMostureWarning(param.asFloat());
}

BLYNK_WRITE(V18)
{
  s_instance->FakeSoilTemperature(param.asFloat());
}

BLYNK_WRITE(V22)
{
  s_instance->FakeSoilMoisture(param.asFloat());
}

BLYNK_WRITE(V23)
{
  s_instance->FakeInsideHumidity(param.asFloat());
}

BLYNK_WRITE(V24)
{
  s_instance->ActiveSwitch(param.asInt());
}

BLYNK_WRITE(V25)
{
  if (param.asInt() == 1) {
    s_instance->ToggleActiveSwitch();
  }
}

BLYNK_WRITE(V31)
{
  s_instance->DayStartHour(param.asInt());
}

BLYNK_WRITE(V32)
{
  s_instance->DayEndHour(param.asInt());
}

BLYNK_WRITE(V34)
{
  s_instance->PvVoltageSensorMin(param.asFloat());
}

BLYNK_WRITE(V35)
{
  s_instance->PvVoltageSensorMax(param.asFloat());
}

BLYNK_WRITE(V36)
{
  s_instance->PvVoltageOutputMin(param.asFloat());
}

BLYNK_WRITE(V37)
{
  s_instance->PvVoltageOutputMax(param.asFloat());
}

BLYNK_WRITE(V38)
{
  s_instance->PvCurrentSensorMin(param.asFloat());
}

BLYNK_WRITE(V39)
{
  s_instance->PvCurrentSensorMax(param.asFloat());
}

BLYNK_WRITE(V40)
{
  s_instance->PvCurrentOutputMin(param.asFloat());
}

BLYNK_WRITE(V41)
{
  s_instance->PvCurrentOutputMax(param.asFloat());
}

BLYNK_WRITE(V44)
{
  s_instance->PvVoltageSwitchOn(param.asFloat());
}

BLYNK_WRITE(V45)
{
  s_instance->PvVoltageSwitchOff(param.asFloat());
}

BLYNK_WRITE(V47)
{
  s_instance->PvForceOn(param.asInt());
}

BLYNK_WRITE(V48)
{
  s_instance->WindowActuatorSpeedPercent(param.asInt());
}

BLYNK_WRITE(V49)
{
  s_instance->WindowActuatorRuntimeSec(param.asFloat());
}

BLYNK_WRITE(V50)
{
  s_instance->DayWaterTemperature(param.asFloat());
}

BLYNK_WRITE(V51)
{
  s_instance->NightWaterTemperature(param.asFloat());
}

BLYNK_WRITE(V53)
{
  s_instance->DaySoilTemperature(param.asFloat());
}

BLYNK_WRITE(V54)
{
  s_instance->NightSoilTemperature(param.asFloat());
}

BLYNK_WRITE(V57)
{
  s_instance->DayAirTemperature(param.asFloat());
}

BLYNK_WRITE(V58)
{
  s_instance->NightAirTemperature(param.asFloat());
}

BLYNK_WRITE(V61)
{
  s_instance->WaterHeaterLimitMinutes(param.asFloat());
}

BLYNK_WRITE(V62)
{
  s_instance->WaterHeatingRuntimeSeconds(param.asInt());

  // TODO: find a better way to always call this last; sometimes
  // when adding new write functions, moving this gets forgotten about.
  s_instance->LastWrite();
}
