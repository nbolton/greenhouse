#include "System.h"

#include <cstdarg>

#include "Heating.h"
#include "ISystem.h"
#include "MultiShiftRegister.h"
#include "common.h"
#include "ho_config.h"

#include <Adafruit_SHT31.h>
#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp8266.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <PCF8574.h>
#include <SPI.h>
#include <Wire.h>

using namespace common;

namespace ng = native::greenhouse;
namespace eg = embedded::greenhouse;

namespace embedded {
namespace greenhouse {

#define SR_ON HIGH // opposite due to bjt
#define SR_OFF LOW 

struct ADC {
  String name;
  ADS1115_WE ads;
  bool ready = false;
};

// regular pins
const int k_oneWirePin = D3;
const int k_shiftRegisterLatchPin = D5;  // RCLK (12)
const int k_shiftRegisterDataPin = D6;   // SER (14)
const int k_shiftRegisterClockPin = D7;  // SRCLK (11)
const int k_shiftRegisterEnablePin = D8; // OE (13)
const bool k_shiftRegisterTestEnable = false;
const int k_shiftRegisterTestFrom = 0; // min 0
const int k_shiftRegisterTestTo = 15;  // max 15
const int k_shiftRegisterTestOn = 500; // time between shift and clear
const int k_shiftRegisterTestOff = 0;  // time between last clear and next shift
const int k_shiftRegisterMinVoltage = 5;

// msr pins
const int k_pvRelayPin = 0;
const int k_BeepPin = 1;
const int k_caseFanPin = 2;
const int k_psuRelayPin = 3;
const int k_batteryLedPin = 4;
const int k_psuLedPin = 5;
const int k_actuatorPin1 = 7; // IN1
const int k_actuatorPin2 = 6; // IN2
const int k_switchPins[] = {0 + 8, 1 + 8, 2 + 8, 3 + 8};

// adc0 pins
const ADS1115_MUX k_psuVoltagePin = ADS1115_COMP_2_GND;
const ADS1115_MUX k_commonVoltagePin = ADS1115_COMP_3_GND;

// adc1 pins
const ADS1115_MUX k_moisturePin = ADS1115_COMP_1_GND;

// adc2 pins
const ADS1115_MUX k_pvVoltagePin = ADS1115_COMP_0_GND;
const ADS1115_MUX k_pvCurrentPin = ADS1115_COMP_1_GND;

const bool k_enableLed = false;
const int k_shiftRegisterTotal = 2;
const int k_ledFlashDelay = 30; // ms
const int k_soilProbeIndex = 0;
const int k_waterProbeIndex = 1;
const uint8_t k_insideAirSensorAddress = 0x44;
const uint8_t k_outsideAirSensorAddress = 0x45;
const int k_adcAddress0 = 0x4A;
const int k_adcAddress1 = 0x48;
const int k_adcAddress2 = 0x49;
const float k_weatherLat = 54.203;
const float k_weatherLon = -4.408;
const char *k_weatherApiKey = "e8444a70abfc2b472d43537730750892";
const char *k_weatherHost = "api.openweathermap.org";
const char *k_weatherUri = "/data/2.5/weather?lat=%.3f&lon=%.3f&units=metric&appid=%s";
const uint8_t k_ioAddress = 0x20;
const int k_loopDelay = 1000;
const int k_blynkFailuresMax = 300;
const int k_blynkRecoverTimeSec = 300; // 5m
const int k_serialWaitDelay = 1000;    // 1s

static System *s_instance = nullptr;
static PCF8574 s_io1(k_ioAddress);
static MultiShiftRegister s_shiftRegisters(
  k_shiftRegisterTotal, k_shiftRegisterLatchPin, k_shiftRegisterClockPin, k_shiftRegisterDataPin);
static OneWire s_oneWire(k_oneWirePin);
static DallasTemperature s_dallas(&s_oneWire);
static char s_reportBuffer[200];
static BlynkTimer s_refreshTimer;
static Adafruit_SHT31 s_insideAirSensor;
static Adafruit_SHT31 s_outsideAirSensor;
static ADC s_adc0, s_adc1, s_adc2;
static WiFiClient s_wifiClient;
static char s_weatherInfo[50];
static DynamicJsonDocument s_weatherJson(2048);
static int s_switchOnCount = 0;

// free-function declarations

void refreshTimer() { eg::s_instance->QueueCallback(&eg::System::Refresh, "Refresh (timer)"); }

// member functions

System &System::Instance() { return *s_instance; }

void System::Instance(System &ga) { s_instance = &ga; }

System::System() :
  m_log(),
  m_heating(),
  m_power(k_psuRelayPin, k_pvRelayPin, k_batteryLedPin, k_psuLedPin),
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
  m_soilMoisture(k_unknown),
  m_insideHumidityWarningSent(false),
  m_soilMoistureWarningSent(false),
  m_activeSwitch(k_unknown),
  m_blynkFailures(0),
  m_lastBlynkFailure(0),
  m_shiftRegisterEnabled(true),
  m_refreshRate(k_unknown),
  m_queueOnSystemStarted(false)
{
  for (int i = 0; i < k_switchCount; i++) {
    m_switchState[i] = false;
  }
}

void System::Setup()
{
  ng::System::Setup();

  if (k_trace) {
    Serial.begin(9600);

    // wait for serial to connect before first trace
    Delay(k_serialWaitDelay, "Serial wait");
  }

  if (k_enableLed) {
    pinMode(LED_BUILTIN, OUTPUT);
  }

  Log().Trace(F("\n\nStarting system: %s"), BLYNK_DEVICE_NAME);

  InitShiftRegisters();

  Beep(1, false);
  CaseFan(true); // on by default

  Wire.begin();

  m_power.Embedded(*this);
  m_power.Native(*this);
  m_power.Setup();

  m_time.Setup();

  s_dallas.begin();

  InitSensors();
  InitADCs();

  m_radio.System(this);
  m_radio.Setup();

  Blynk.begin(k_auth, k_ssid, k_pass);

  ReportSwitchStates();

  Log().Trace(F("System ready"));
  Blynk.virtualWrite(V52, "Ready");
  Beep(2, false);
}

void System::Loop()
{
  // HACK: the shift register gets into an undefined state sometimes,
  // so keep shifting to ensure that it's state is persisted.
  s_shiftRegisters.shift();

  m_radio.Loop();
  if (m_radio.Busy()) {
    return;
  }

  // always run before actuator check
  ng::System::Loop();

  if (IsWindowActuatorRunning()) {
    return;
  }

  if (Serial.available() > 0) {
    String s = Serial.readString();
    const char c = s.c_str()[0];
    switch (c) {
    case 's':
      PrintStatus();
      break;

    default:
      PrintCommands();
      break;
    }
  }

  unsigned long now = Time().EpochTime();
  if (!Blynk.run()) {

    m_lastBlynkFailure = now;
    m_blynkFailures++;
    Log().Trace(F("Blynk failed %d time(s)"), m_blynkFailures);

    if (m_blynkFailures >= k_blynkFailuresMax) {
      Log().Trace(F("Restarting, too many Blynk failures"));
      Restart();
      return;
    }
  }
  else if (((now - m_lastBlynkFailure) > k_blynkRecoverTimeSec) && (m_blynkFailures != 0)) {
    // no failures happened within last n - r seconds (n = now, r = recover time)
    Log().Trace(F("Blynk is back to normal, resetting fail count"));
    m_blynkFailures = 0;
  }

  Power().Loop();

  // the shift register seems to draw power through from the microcontroller,
  // possibly through the inputs, which doesn't seem great. so, turn the shift
  // register off when it doesn't have any power. ideally this should be done
  // though circuit logic, but this will do for now.
  const bool srEnable = m_power.ReadCommonVoltage() >= k_shiftRegisterMinVoltage;
  if (srEnable != m_shiftRegisterEnabled) {
    Log().Trace(F("Shift register %s"), srEnable ? "enabled" : "disabled");
    digitalWrite(k_shiftRegisterEnablePin, srEnable ? SR_ON : SR_OFF);

    m_shiftRegisterEnabled = srEnable;
  }

  while (!m_toggleActiveSwitchQueue.empty()) {
    int queuedSwitch = m_toggleActiveSwitchQueue.front();
    m_toggleActiveSwitchQueue.pop();

    ToggleSwitch(queuedSwitch);
  }

  // may or may not queue a refresh
  s_refreshTimer.run();

  if (SystemStarted()) {
    while (!m_callbackQueue.empty()) {
      Callback callback = m_callbackQueue.front();

      Log().Trace("Callback function: %s", callback.m_name.c_str());

      m_callbackQueue.pop();

      if (callback.m_function != nullptr) {
        (this->*callback.m_function)();
      }
      else {
        Log().Trace("Function pointer was null");
      }

      Log().Trace("Callback queue remaining=%d", m_callbackQueue.size());
    }
  }

  if (m_queueOnSystemStarted) {
    m_queueOnSystemStarted = false;
    OnSystemStarted();
  }

#ifdef LOOP_DELAY
  // slow loop down to save power
  Delay(k_loopDelay, "Loop");
#endif
}

void System::InitShiftRegisters()
{
  pinMode(k_shiftRegisterLatchPin, OUTPUT);
  pinMode(k_shiftRegisterClockPin, OUTPUT);
  pinMode(k_shiftRegisterDataPin, OUTPUT);

  pinMode(k_shiftRegisterEnablePin, OUTPUT);
  digitalWrite(k_shiftRegisterEnablePin, SR_ON);

  Log().Trace(F("Init shift"));
  s_shiftRegisters.shift();

  if (k_shiftRegisterTestEnable) {
    digitalWrite(k_shiftRegisterEnablePin, SR_ON);
    const int from = k_shiftRegisterTestFrom;
    const int to = k_shiftRegisterTestTo + 1;
    while (true) {
      Log().Trace(F("Test shift loop from=%d to=%d"), from, to);
      for (int i = from; i < to; i++) {

        Log().Trace(F("Test set SR pin %d"), i);
        s_shiftRegisters.set_shift(i);
        Delay(k_shiftRegisterTestOn, "SR test on");

        Log().Trace(F("Test clear SR pin %d"), i);
        s_shiftRegisters.clear_shift(i);
        Delay(k_shiftRegisterTestOff, "SR test off");
      }
    }
  }
}

void System::InitSensors()
{
  if (!s_insideAirSensor.begin(k_insideAirSensorAddress)) {
    ReportWarning("Inside air sensor not found");
  }

  if (!s_outsideAirSensor.begin(k_outsideAirSensorAddress)) {
    ReportWarning("Inside air sensor not found");
  }
}

void System::InitADCs()
{
  s_adc0.name = "ADS1115 #0";
  s_adc1.name = "ADS1115 #1";
  s_adc2.name = "ADS1115 #2";
  s_adc0.ads = ADS1115_WE(k_adcAddress0);
  s_adc1.ads = ADS1115_WE(k_adcAddress1);
  s_adc2.ads = ADS1115_WE(k_adcAddress2);
  s_adc0.ready = s_adc0.ads.init();
  s_adc1.ready = s_adc1.ads.init();
  s_adc2.ready = s_adc2.ads.init();

  if (!s_adc0.ready) {
    ReportWarning("ADC not ready, init failed: %s", s_adc0.name.c_str());
  }

  if (!s_adc1.ready) {
    ReportWarning("ADC not ready, init failed: %s", s_adc1.name.c_str());
  }

  if (!s_adc2.ready) {
    ReportWarning("ADC not ready, init failed: %s", s_adc2.name.c_str());
  }
}

void System::Refresh()
{
  Log().Trace(F("Refreshing (embedded)"));

  Time().Refresh();

  FlashLed(k_ledRefresh);

  ng::System::Refresh();

  m_power.MeasureCurrent();

  Log().Trace(F("Common voltage: %.2fV"), m_power.ReadCommonVoltage());
  Log().Trace(F("PSU voltage: %.2fV"), m_power.ReadPsuVoltage());

  Blynk.virtualWrite(V28, Power().PvPowerSource());
  Blynk.virtualWrite(V29, Power().PvVoltageSensor());
  Blynk.virtualWrite(V30, Power().PvVoltageOutput());
  Blynk.virtualWrite(V42, Power().PvCurrentSensor());
  Blynk.virtualWrite(V43, Power().PvCurrentOutput());
  Blynk.virtualWrite(V55, Heating().WaterHeaterIsOn());
  Blynk.virtualWrite(V56, Heating().SoilHeatingIsOn());
  Blynk.virtualWrite(V59, Heating().AirHeatingIsOn());

  Log().Trace(F("Refresh done (embedded)"));
}

void System::FlashLed(LedFlashTimes times)
{
  if (!k_enableLed) {
    return;
  }

  for (int i = 0; i < (int)times * 2; i++) {
    m_led = (m_led == LOW) ? HIGH : LOW;
    digitalWrite(LED_BUILTIN, m_led);
    Delay(k_ledFlashDelay, "LED flash");
  }
}

float System::InsideAirHumidity() const
{
  if (TestMode()) {
    return m_fakeInsideHumidity;
  }
  return m_insideAirHumidity;
}

float System::SoilTemperature() const
{
  if (TestMode()) {
    return m_fakeSoilTemperature;
  }
  return m_soilTemperature;
}

float System::SoilMoisture() const
{
  if (TestMode()) {
    return m_fakeSoilMoisture;
  }
  return m_soilMoisture;
}

bool System::ReadSensors(int &failures)
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

  bool dallasOk = false;
  int dallasRetry = 0;
  const int dallasRetryMax = 5;
  const int dallasRetryWait = 100;

  while (!dallasOk) {

    s_dallas.requestTemperatures();

    m_soilTemperature = s_dallas.getTempCByIndex(k_soilProbeIndex);
    m_waterTemperature = s_dallas.getTempCByIndex(k_waterProbeIndex);

    dallasOk = true;
    if (m_soilTemperature == DEVICE_DISCONNECTED_C) {
      m_soilTemperature = k_unknown;
      failures++;
      dallasOk = false;
    }

    if (m_waterTemperature == DEVICE_DISCONNECTED_C) {
      m_waterTemperature = k_unknown;
      failures++;
      dallasOk = false;
    }

    if (dallasOk || (++dallasRetry > dallasRetryMax)) {
      break;
    }

    Log().Trace(F("Dallas sensor read failed, retrying (attempt %d)"), dallasRetry);
    Delay(dallasRetryWait, "Dallas retry");
  }

  ReadSoilMoistureSensor();
  if (SoilSensor() == k_unknown) {
    m_soilMoisture = k_unknown;
    failures++;
  }
  else {
    float moisture = CalculateMoisture(SoilSensor());
    if (moisture == k_unknown) {
      failures++;
    }
    else {
      AddSoilMoistureSample(moisture);
      m_soilMoisture = SoilMoistureAverage();
    }
  }

  return failures == 0;
}

bool System::ReadSoilMoistureSensor()
{
  SoilSensor(ReadAdc(s_adc1, k_moisturePin));
  return (SoilSensor() != k_unknown);
}

void System::RunWindowActuator(bool extend)
{
  if (extend) {
    Log().Trace(F("Actuator set IN1, clear IN2"));
    s_shiftRegisters.set(k_actuatorPin1);
    s_shiftRegisters.clear(k_actuatorPin2);
  }
  else {
    Log().Trace(F("Actuator clear IN1, set IN2"));
    s_shiftRegisters.clear(k_actuatorPin1);
    s_shiftRegisters.set(k_actuatorPin2);
  }
  s_shiftRegisters.shift();
}

void System::StopActuator()
{
  Log().Trace(F("Stopping actuator"));
  s_shiftRegisters.clear(k_actuatorPin1);
  s_shiftRegisters.clear(k_actuatorPin2);
  s_shiftRegisters.shift();
}

void System::Delay(unsigned long ms, const char *reason)
{
  Log().Trace(F("%s delay: %dms"), reason, (int)ms);
  delay(ms);
}

void System::Restart()
{
  FlashLed(k_ledRestart);
  Beep(2, true);

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

void System::CaseFan(bool on)
{
  Log().Trace(F("Case fan %s"), on ? "on" : "off");

  if (on) {
    s_shiftRegisters.set(k_caseFanPin);
  }
  else {
    s_shiftRegisters.clear(k_caseFanPin);
  }

  Log().Trace(F("Case fan shift"));
  s_shiftRegisters.shift();
}

void System::Beep(int times, bool longBeep)
{
  for (int i = 0; i < times; i++) {
    Log().Trace(F("Beep set shift"));
    s_shiftRegisters.set_shift(k_BeepPin);
    Delay(longBeep ? 200 : 100, "Beep on");

    Log().Trace(F("Beep clear shift"));
    s_shiftRegisters.clear_shift(k_BeepPin);
    Delay(200, "Beep off");
  }
}

void System::ReportSwitchStates()
{
  String switchStates;
  s_switchOnCount = 0;
  for (int i = 0; i < k_switchCount; i++) {
    bool on = m_switchState[i];
    if (on) {
      s_switchOnCount++;
    }
    switchStates += "S";
    switchStates += i;
    switchStates += "=";
    switchStates += on ? "On" : "Off";
    if (i != (k_switchCount - 1)) {
      switchStates += ", ";
    }
  }
  Blynk.virtualWrite(V33, switchStates);
}

void System::SetSwitch(int index, bool on)
{
  int pin = k_switchPins[index];

  if (on) {
    Log().Trace(F("Switch set: %d"), pin);
    s_shiftRegisters.set(pin);
    m_switchState[index] = true;
  }
  else {
    Log().Trace(F("Switch clear: %d"), pin);
    s_shiftRegisters.clear(pin);
    m_switchState[index] = false;
  }

  Log().Trace(F("Switch shift"));
  s_shiftRegisters.shift();

  ReportSwitchStates();

  UpdateCaseFan();
}

void System::ToggleSwitch(int switchIndex)
{
  if (!m_switchState[switchIndex]) {
    Log().Trace(F("Toggle switch on: %d"), switchIndex);
    SetSwitch(switchIndex, true);
  }
  else {
    Log().Trace(F("Toggle switch off: %d"), switchIndex);
    SetSwitch(switchIndex, false);
  }
}

float System::ReadAdc(ADC &adc, ADS1115_MUX channel)
{
  if (!adc.ready) {
    Log().Trace(F("ADC not ready: %s"), adc.name.c_str());
    return k_unknown;
  }

  // HACK: this keeps getting reset to default (possibly due to a power issue
  // when the relay switches), so force the volt range every time we're about to read.
  s_adc0.ads.setVoltageRange_mV(ADS1115_RANGE_6144);
  s_adc1.ads.setVoltageRange_mV(ADS1115_RANGE_6144);
  s_adc2.ads.setVoltageRange_mV(ADS1115_RANGE_6144);

  adc.ads.setCompareChannels(channel);
  adc.ads.startSingleMeasurement();

#ifdef ADC_WAIT
  int times = 0;
  while (adc.ads.isBusy()) {
    Delay(10, "ADC wait");
    if (times++ > 10) {
      Log().Trace(F("ADC is busy: %s"), adc.name.c_str());
      return k_unknown;
    }
  }
#endif

  return adc.ads.getResult_V(); // alternative: getResult_mV for Millivolt
}

void System::ReportInfo(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  Log().Trace(s_reportBuffer);
  Blynk.logEvent("info", s_reportBuffer);
}

void System::ReportWarning(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  // still log even if k_reportWarnings is off (the intent of k_reportWarnings
  // is to reduce noise on the blynk app during testing)
  Log().Trace(s_reportBuffer);

  if (!k_reportWarnings) {
    return;
  }

  Blynk.logEvent("warning", s_reportBuffer);
}

void System::ReportCritical(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  Log().Trace(s_reportBuffer);
  Blynk.logEvent("critical", s_reportBuffer);
}

void System::ReportSensorValues()
{
  Blynk.virtualWrite(V1, InsideAirTemperature());
  Blynk.virtualWrite(V2, InsideAirHumidity());
  Blynk.virtualWrite(V19, OutsideAirTemperature());
  Blynk.virtualWrite(V20, OutsideAirHumidity());
  Blynk.virtualWrite(V11, SoilTemperature());
  Blynk.virtualWrite(V21, SoilMoistureAverage());
  Blynk.virtualWrite(V46, WaterTemperature());
}

void System::ReportWindowProgress() { Blynk.virtualWrite(V9, WindowProgressExpected()); }

void System::ReportSystemInfo()
{
  // current time
  Blynk.virtualWrite(V10, Time().FormattedCurrentTime());

  // uptime
  Blynk.virtualWrite(V12, Time().FormattedUptime());

  // free heap
  int freeHeap = ESP.getFreeHeap();
  Log().Trace(F("Free heap: %d bytes"), freeHeap);
  Blynk.virtualWrite(V13, (float)freeHeap / 1000);
}

void System::HandleNightToDayTransition()
{
  // report cumulative time to daily virtual pin before calling
  // base function (which resets cumulative to 0)
  Blynk.virtualWrite(V65, Heating().WaterHeaterCostCumulative());

  ng::System::HandleNightToDayTransition();

  m_soilMoistureWarningSent = false;
  m_insideHumidityWarningSent = false;

  Blynk.virtualWrite(V64, Time().NightToDayTransitionTime());
}

void System::HandleDayToNightTransition()
{
  ng::System::HandleDayToNightTransition();

  Blynk.virtualWrite(V68, Time().DayToNightTransitionTime());
}

void System::ReportWarnings()
{
  if (!SystemStarted()) {
    return;
  }

  bool moistureKnown = (m_soilMoisture != k_unknown);
  bool moistureLow = (m_soilMoisture <= SoilMostureWarning());
  if (!m_soilMoistureWarningSent && moistureKnown && moistureLow) {
    ReportWarning("Soil moisture low (%.2f%%)", m_soilMoisture);
    m_soilMoistureWarningSent = true;
  }
}

void System::OnLastWrite()
{
  if (!SystemStarted()) {
    m_queueOnSystemStarted = true;
  }
}

void System::OnSystemStarted()
{
  Power().InitPowerSource();

  // run first refresh (instead of waiting for the 1st refresh timer).
  // we run the 1st refresh here instead of when the timer is created,
  // because when we setup the timer for the first time, we may not
  // have all of the correct initial values.
  Log().Trace(F("First refresh"));
  Refresh();

  FlashLed(k_ledStarted);
  SystemStarted(true);

  // system started may sound like a good thing, but actually the system
  // shouldn't normally stop. so, that it has started is not usually a
  // good thing (unless we're in test mode, but then warnings are disabled).
  ReportWarning("System started");
}

void System::ApplyRefreshRate()
{
  if (m_refreshRate <= 0) {
    Log().Trace(F("Invalid refresh rate: %ds"), m_refreshRate);
    return;
  }

  Log().Trace(F("New refresh rate: %ds"), m_refreshRate);

  if (m_timerId != k_unknown) {
    Log().Trace(F("Deleting old timer: %d"), m_timerId);
    s_refreshTimer.deleteTimer(m_timerId);
  }

  m_timerId = s_refreshTimer.setInterval(m_refreshRate * 1000L, refreshTimer);
  Log().Trace(F("New refresh timer: %d"), m_timerId);
}

bool System::UpdateWeatherForecast()
{
  char uri[100];
  sprintf(uri, k_weatherUri, k_weatherLat, k_weatherLon, k_weatherApiKey);

  Log().Trace(F("Connecting to weather host: %s"), k_weatherHost);
  HttpClient httpClient(s_wifiClient, k_weatherHost, 80);

  Log().Trace(F("Weather host get"));
  httpClient.get(uri);

  int statusCode = httpClient.responseStatusCode();
  if (isnan(statusCode)) {
    Log().Trace(F("Weather host status is invalid"));
    return false;
  }

  Log().Trace(F("Weather host status: %d"), statusCode);
  if (statusCode != 200) {
    Log().Trace(F("Weather host error status"));
    return false;
  }

  String response = httpClient.responseBody();
  int size = static_cast<int>(strlen(response.c_str()));
  Log().Trace(F("Weather host response length: %d"), size);

  DeserializationError error = deserializeJson(s_weatherJson, response);
  if (error != DeserializationError::Ok) {
    Log().Trace(F("Weather data error: %s"), error.c_str());
    return false;
  }

  Log().Trace(F("Deserialized weather JSON"));
  int id = s_weatherJson["weather"][0]["id"];
  const char *main = s_weatherJson["weather"][0]["main"];
  int dt = s_weatherJson["dt"];
  const char *location = s_weatherJson["name"];

  int hours = (int)((dt % 86400L) / 3600);
  int minutes = (int)((dt % 3600) / 60);

  String hoursString = hours < 10 ? "0" + String(hours) : String(hours);
  String minuteString = minutes < 10 ? "0" + String(minutes) : String(minutes);

  sprintf(
    s_weatherInfo,
    "%s @ %s:%s UTC (%s)",
    main,
    hoursString.c_str(),
    minuteString.c_str(),
    location);

  Log().Trace(F("Weather forecast: code=%d, info='%s'"), id, s_weatherInfo);

  WeatherCode(id);
  WeatherInfo(s_weatherInfo);
  ReportWeather();
  return true;
}

void System::ReportWeather() { Blynk.virtualWrite(V60, WeatherInfo().c_str()); }

void System::ReportWaterHeaterInfo()
{
  Blynk.virtualWrite(V62, Heating().WaterHeaterDayRuntimeMinutes());
  Blynk.virtualWrite(V67, Heating().WaterHeaterNightRuntimeMinutes());
  Blynk.virtualWrite(V63, Heating().WaterHeaterCostCumulative());
}

void System::ReportMoistureCalibration()
{
  Blynk.virtualWrite(V69, SoilSensorWet());
  Blynk.virtualWrite(V70, SoilSensorDry());
}

void System::UpdateCaseFan()
{
  // turn case fan on when any switch is on or if PSU is in use
  bool caseFanOn = (s_switchOnCount > 0) || (!Power().PvPowerSource());
  CaseFan(caseFanOn);
}

void System::OnPowerSwitch()
{
  UpdateCaseFan();
  Blynk.virtualWrite(V28, Power().PvPowerSource());
}

void System::ExpanderWrite(int pin, int value) { s_io1.digitalWrite(pin, value); }

void System::ShiftRegister(int pin, bool set)
{
  if (set) {
    s_shiftRegisters.set_shift(pin);
  }
  else {
    s_shiftRegisters.clear_shift(pin);
  }
}

bool System::PowerSensorReady() { return s_adc2.ready; }

float System::ReadPowerSensorVoltage() { return ReadAdc(s_adc2, k_pvVoltagePin); }

float System::ReadPowerSensorCurrent() { return ReadAdc(s_adc2, k_pvCurrentPin); }

float System::ReadCommonVoltageSensor() { return ReadAdc(s_adc0, k_commonVoltagePin); }

float System::ReadPsuVoltageSensor() { return ReadAdc(s_adc0, k_psuVoltagePin); }

void System::PrintCommands() { Log().Trace(F("Commands\ns: status")); }

void System::PrintStatus()
{
  Log().Trace(F("Status\nBlynk: %s"), Blynk.connected() ? "Connected" : "Disconnected");
}

void System::QueueToggleActiveSwitch() { m_toggleActiveSwitchQueue.push(m_activeSwitch); }

void System::QueueCallback(CallbackFunction f, std::string name)
{
  m_callbackQueue.push(Callback(f, name));
}

} // namespace greenhouse
} // namespace embedded

// blynk

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
    V62,
    V63,
    V64,
    V66,
    V67,
    V68,
    V69,
    V70,
    V71,
    V72,
    V73,
    V74,
    V127 /* last */);
}

BLYNK_WRITE(V0) { eg::s_instance->AutoMode(param.asInt() == 1); }

BLYNK_WRITE(V3)
{
  eg::s_instance->RefreshRate(param.asInt());
  eg::s_instance->QueueCallback(&eg::System::ApplyRefreshRate, "Refresh rate");
}

BLYNK_WRITE(V5) { eg::s_instance->OpenStart(param.asFloat()); }

BLYNK_WRITE(V6)
{
  if (param.asInt() == 1) {
    eg::s_instance->QueueCallback(&eg::System::Restart, "Restart");
  }
}

BLYNK_WRITE(V7)
{
  if (param.asInt() == 1) {
    eg::s_instance->QueueCallback(&eg::System::Refresh, "Refresh (manual)");
  }
}

BLYNK_WRITE(V8) { eg::s_instance->OpenFinish(param.asFloat()); }

BLYNK_WRITE(V9)
{
  eg::s_instance->WindowProgress(param.asInt());
  eg::s_instance->QueueCallback(&eg::System::UpdateWindowProgress, "Window progress");
}

BLYNK_WRITE(V14) { eg::s_instance->TestMode(param.asInt() == 1); }

BLYNK_WRITE(V17) { eg::s_instance->SoilMostureWarning(param.asFloat()); }

BLYNK_WRITE(V18) { eg::s_instance->FakeSoilTemperature(param.asFloat()); }

BLYNK_WRITE(V22) { eg::s_instance->FakeSoilMoisture(param.asFloat()); }

BLYNK_WRITE(V23) { eg::s_instance->FakeInsideHumidity(param.asFloat()); }

BLYNK_WRITE(V24) { eg::s_instance->ActiveSwitch(param.asInt()); }

BLYNK_WRITE(V25)
{
  if (param.asInt() == 1) {
    eg::s_instance->QueueToggleActiveSwitch();
  }
}

BLYNK_WRITE(V31) { eg::s_instance->Time().DayStartHour(param.asInt()); }

BLYNK_WRITE(V32) { eg::s_instance->Time().DayEndHour(param.asInt()); }

BLYNK_WRITE(V34) { eg::s_instance->Power().PvVoltageSensorMin(param.asFloat()); }

BLYNK_WRITE(V35) { eg::s_instance->Power().PvVoltageSensorMax(param.asFloat()); }

BLYNK_WRITE(V36) { eg::s_instance->Power().PvVoltageOutputMin(param.asFloat()); }

BLYNK_WRITE(V37) { eg::s_instance->Power().PvVoltageOutputMax(param.asFloat()); }

BLYNK_WRITE(V38) { eg::s_instance->Power().PvCurrentSensorMin(param.asFloat()); }

BLYNK_WRITE(V39) { eg::s_instance->Power().PvCurrentSensorMax(param.asFloat()); }

BLYNK_WRITE(V40) { eg::s_instance->Power().PvCurrentOutputMin(param.asFloat()); }

BLYNK_WRITE(V41) { eg::s_instance->Power().PvCurrentOutputMax(param.asFloat()); }

BLYNK_WRITE(V44) { eg::s_instance->Power().PvVoltageSwitchOn(param.asFloat()); }

BLYNK_WRITE(V45) { eg::s_instance->Power().PvVoltageSwitchOff(param.asFloat()); }

BLYNK_WRITE(V47) { eg::s_instance->Power().PvMode((embedded::greenhouse::PvModes)param.asInt()); }

BLYNK_WRITE(V48) { eg::s_instance->WindowAdjustPositions(param.asInt()); }

BLYNK_WRITE(V49) { eg::s_instance->WindowActuatorRuntimeSec(param.asFloat()); }

BLYNK_WRITE(V50) { eg::s_instance->Heating().DayWaterTemperature(param.asFloat()); }

BLYNK_WRITE(V51) { eg::s_instance->Heating().NightWaterTemperature(param.asFloat()); }

BLYNK_WRITE(V53) { eg::s_instance->Heating().DaySoilTemperature(param.asFloat()); }

BLYNK_WRITE(V54) { eg::s_instance->Heating().NightSoilTemperature(param.asFloat()); }

BLYNK_WRITE(V57) { eg::s_instance->Heating().DayAirTemperature(param.asFloat()); }

BLYNK_WRITE(V58) { eg::s_instance->Heating().NightAirTemperature(param.asFloat()); }

BLYNK_WRITE(V61) { eg::s_instance->Heating().WaterHeaterDayLimitMinutes(param.asInt()); }

BLYNK_WRITE(V62) { eg::s_instance->Heating().WaterHeaterDayRuntimeMinutes(param.asFloat()); }

BLYNK_WRITE(V63) { eg::s_instance->Heating().WaterHeaterCostCumulative(param.asFloat()); }

BLYNK_WRITE(V64)
{
  if (!String(param.asString()).isEmpty()) {
    eg::s_instance->Time().NightToDayTransitionTime(param.asLongLong());
  }
  else {
    eg::s_instance->Time().NightToDayTransitionTime(k_unknownUL);
  }
}

BLYNK_WRITE(V66) { eg::s_instance->Heating().WaterHeaterNightLimitMinutes(param.asInt()); }

BLYNK_WRITE(V67) { eg::s_instance->Heating().WaterHeaterNightRuntimeMinutes(param.asFloat()); }

BLYNK_WRITE(V68)
{
  if (!String(param.asString()).isEmpty()) {
    eg::s_instance->Time().DayToNightTransitionTime(param.asLongLong());
  }
  else {
    eg::s_instance->Time().DayToNightTransitionTime(k_unknownUL);
  }
}

BLYNK_WRITE(V69) { eg::s_instance->SoilSensorWet(param.asFloat()); }

BLYNK_WRITE(V70) { eg::s_instance->SoilSensorDry(param.asFloat()); }

BLYNK_WRITE(V71)
{
  if (param.asInt() != 0) {
    eg::s_instance->QueueCallback(&eg::System::SoilCalibrateWet, "Soil calibrate (wet)");
  }
}

BLYNK_WRITE(V72)
{
  if (param.asInt() != 0) {
    eg::s_instance->QueueCallback(&eg::System::SoilCalibrateDry, "Soil calibrate (dry)");
  }
}

BLYNK_WRITE(V73) { eg::s_instance->Heating().Enabled(param.asInt() == 1); }

BLYNK_WRITE(V74) { eg::s_instance->WindowAdjustTimeframe(param.asInt()); }

// used only as the last value
BLYNK_WRITE(V127) { eg::s_instance->OnLastWrite(); }
