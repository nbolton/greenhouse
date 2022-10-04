#include "System.h"

#include <cstdarg>

#include "Heating.h"
#include "ISystem.h"
#include "MultiShiftRegister.h"
#include "common.h"
#include "gh_config.h"

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

#define SR_ON LOW
#define SR_OFF HIGH

#define ADC_RETRY_MAX 5
#define ADC_RETRY_DELAY 50
#define DEBUG_DELAY 0
#define ADC_DEBUG 0

struct ADC {
  String name;
  ADS1115_WE ads;
  bool ready = false;
};

// regular pins
// const int k_oneWirePin = D3;
const int k_shiftRegisterEnablePin = D5;     // OE (13)
const int k_shiftRegisterSerialPin = D6;     // SER (14)
const int k_shiftRegisterShiftClockPin = D7; // SRCLK (11)
const int k_shiftRegisterStoreClockPin = D8; // RCLK (12)
const bool k_shiftRegisterTestEnable = false;
const int k_shiftRegisterTestFrom = 0; // min 0
const int k_shiftRegisterTestTo = 15;  // max 15
const int k_shiftRegisterTestOn = 500; // time between shift and clear
const int k_shiftRegisterTestOff = 0;  // time between last clear and next shift
const int k_shiftRegisterMinVoltage = 5;

// msr pins
const int k_batteryRelayPin = 0;
const int k_BeepPin = 1;
const int k_caseFanPin = 2;
const int k_psuRelayPin = 3;
const int k_batteryLedPin = 4;
const int k_psuLedPin = 5;
const int k_actuatorPin1 = 7; // IN1
const int k_actuatorPin2 = 6; // IN2
const int k_switchPins[] = {0 + 8, 1 + 8, 2 + 8, 3 + 8};

// adc1 pins
const ADS1115_MUX k_localVoltagePin = ADS1115_COMP_3_GND;

// adc2 pins
const ADS1115_MUX k_batteryVoltagePin = ADS1115_COMP_0_GND;
const ADS1115_MUX k_batteryCurrentPin = ADS1115_COMP_1_GND;

const bool k_enableLed = false;
const int k_shiftRegisterTotal = 2;
const int k_ledFlashDelay = 30; // ms
const int k_soilProbeIndex = 0;
const int k_waterProbeIndex = 1;
const uint8_t k_insideAirSensorAddress = 0x44;
const uint8_t k_outsideAirSensorAddress = 0x45;
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
  k_shiftRegisterTotal,
  k_shiftRegisterStoreClockPin,
  k_shiftRegisterShiftClockPin,
  k_shiftRegisterSerialPin);
static char s_reportBuffer[200];
static BlynkTimer s_refreshTimer;
static Adafruit_SHT31 s_insideAirSensor;
static Adafruit_SHT31 s_outsideAirSensor;
static ADC s_adc1, s_adc2;
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
  m_heating(),
  m_power(k_psuRelayPin, k_batteryRelayPin, k_batteryLedPin, k_psuLedPin),
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
  m_refreshRate(k_unknown),
  m_queueOnSystemStarted(false),
  m_lastLoop(0)
{
  for (int i = 0; i < k_switchCount; i++) {
    m_switchState[i] = false;
  }
}

void System::Setup()
{
  ng::System::Setup();

#if TRACE_EN
  if (k_trace) {
    Serial.begin(9600);
    delay(k_serialWaitDelay);
    Serial.println("\n");
  }
#endif // TRACE_EN

  if (k_enableLed) {
    pinMode(LED_BUILTIN, OUTPUT);
  }

  TRACE_F("Starting system: %s", BLYNK_DEVICE_NAME);

  InitShiftRegisters();

#if RADIO_EN
  m_radio.Init(this);
#endif

  Beep(1, false);
  CaseFan(true); // on by default

  Wire.begin();

  m_power.Embedded(*this);
  m_power.Native(*this);
  m_power.Setup();

  m_time.Setup();

  InitSensors();
  InitADCs();

  Blynk.begin(k_auth, k_ssid, k_pass);

  ReportSwitchStates();

  TRACE("System ready");
  Blynk.virtualWrite(V52, "Ready");
  Beep(2, false);
}

void System::Loop()
{
  if (millis() < (m_lastLoop + k_loopDelay)) {
    return;
  }
  m_lastLoop = millis();

  // HACK: the shift register gets into an undefined state sometimes,
  // so keep shifting to ensure that it's state is persisted.
  s_shiftRegisters.shift();

  // always run before actuator check
  ng::System::Loop();

  m_radio.Update();

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

  const unsigned long now = Time().UptimeSeconds();
  const bool recoveredWithinTimeframe = ((now - m_lastBlynkFailure) > k_blynkRecoverTimeSec);

  if (!Blynk.run()) {

    m_lastBlynkFailure = now;
    m_blynkFailures++;
    TRACE_F("Blynk failed %d time(s)", m_blynkFailures);

    if (m_blynkFailures >= k_blynkFailuresMax) {
      TRACE("Restarting, too many Blynk failures");
      Restart();
      return;
    }
  }
  else if (recoveredWithinTimeframe && (m_blynkFailures != 0)) {
    // no failures happened within last n - r seconds (n = now, r = recover time)
    TRACE("Blynk is back to normal, resetting fail count");
    m_blynkFailures = 0;
  }

  Power().Loop();

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

      TRACE_F("Callback function: %s", callback.m_name.c_str());

      m_callbackQueue.pop();

      if (callback.m_function != nullptr) {
        (this->*callback.m_function)();
      }
      else {
        TRACE("Function pointer was null");
      }

      TRACE_F("Callback queue remaining=%d", m_callbackQueue.size());
    }
  }

  if (m_queueOnSystemStarted) {
    m_queueOnSystemStarted = false;
    OnSystemStarted();
  }
}

void System::InitShiftRegisters()
{
  pinMode(k_shiftRegisterStoreClockPin, OUTPUT);
  pinMode(k_shiftRegisterShiftClockPin, OUTPUT);
  pinMode(k_shiftRegisterSerialPin, OUTPUT);

  pinMode(k_shiftRegisterEnablePin, OUTPUT);
  digitalWrite(k_shiftRegisterEnablePin, SR_ON);

  TRACE("Init shift");
  s_shiftRegisters.shift();

  if (k_shiftRegisterTestEnable) {
    digitalWrite(k_shiftRegisterEnablePin, SR_ON);
    const int from = k_shiftRegisterTestFrom;
    const int to = k_shiftRegisterTestTo + 1;
    while (true) {
      TRACE_F("Test shift loop from=%d to=%d", from, to);
      for (int i = from; i < to; i++) {

        TRACE_F("Test set SR pin %d", i);
        s_shiftRegisters.set_shift(i);
        Delay(k_shiftRegisterTestOn, "SR test on");

        TRACE_F("Test clear SR pin %d", i);
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
    ReportWarning("Outside air sensor not found");
  }
}

void System::InitADCs()
{
  s_adc1.name = "ADS1115 #1 - Onboard";
  s_adc1.ads = ADS1115_WE(k_adcAddress1);
  s_adc1.ready = s_adc1.ads.init();
  if (!s_adc1.ready) {
    ReportWarning("ADC not ready, init failed: %s", s_adc1.name.c_str());
  }

  s_adc2.name = "ADS1115 #2 - Battery";
  s_adc2.ads = ADS1115_WE(k_adcAddress2);
  s_adc2.ready = s_adc2.ads.init();
  if (!s_adc2.ready) {
    ReportWarning("ADC not ready, init failed: %s", s_adc2.name.c_str());
  }
}

void System::Refresh()
{
  TRACE("Refreshing (embedded)");

  Time().Refresh();

  FlashLed(k_ledRefresh);

  ng::System::Refresh();

  m_power.MeasureCurrent();

  TRACE_F("Local voltage: %.2fV", m_power.ReadLocalVoltage());

  Blynk.virtualWrite(V28, Power().BatteryPowerSource());
  Blynk.virtualWrite(V29, Power().BatteryVoltageSensor());
  Blynk.virtualWrite(V30, Power().BatteryVoltageOutput());
  Blynk.virtualWrite(V42, Power().BatteryCurrentSensor());
  Blynk.virtualWrite(V43, Power().BatteryCurrentOutput());
  Blynk.virtualWrite(V55, Heating().WaterHeaterIsOn());
  Blynk.virtualWrite(V56, Heating().SoilHeatingIsOn());
  Blynk.virtualWrite(V59, Heating().AirHeatingIsOn());

  TRACE("Refresh done (embedded)");
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
    TRACE("Test mode: Skipping sensor read");
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

  TRACE("Reading soil temperatures");
  radio::Node &rightWindow = m_radio.Node(radio::k_nodeRightWindow);
  const int tempDevs = rightWindow.GetTempDevs();
  TRACE_F("Soil temperatures devices: %d", tempDevs);
  float tempSum = 0;
  for (int i = 0; i < tempDevs; i++) {
    const float t = rightWindow.GetTemp(i);
    if (t != k_unknown) {
      tempSum += t;
      TRACE_F("Soil temperature sensor %d: %.2f", i, t);
    }
    else {
      TRACE_F("Soil temperature sensor %d: unknown", i);
    }
  }

  if (tempSum > 0) {
    m_soilTemperature = tempSum / tempDevs;
    TRACE_F("Soil temperature average: %.2f", m_soilTemperature);
  }
  else {
    m_soilTemperature = k_unknown;
    TRACE("Soil temperatures unknown");
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
  SoilSensor(k_unknown);
  return (SoilSensor() != k_unknown);
}

void System::RunWindowActuator(bool extend, int runtimeSec)
{
  // radio::Node& left = m_radio.Node(radio::k_nodeLeftWindow);
  radio::Node &right = m_radio.Node(radio::k_nodeRightWindow);
  if (extend) {
    // left.MotorRun(radio::k_windowExtend, runtimeSec);
    right.MotorRun(radio::k_windowExtend, runtimeSec);
  }
  else {
    // left.MotorRun(radio::k_windowRetract, runtimeSec);
    right.MotorRun(radio::k_windowRetract, runtimeSec);
  }
}

void System::Delay(unsigned long ms, const char *reason)
{
#if DEBUG_DELAY
  TRACE_F("%s delay: %dms", reason, (int)ms);
#endif // DEBUG_DELAY
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
  TRACE_F("Case fan %s", on ? "on" : "off");

  if (on) {
    s_shiftRegisters.set(k_caseFanPin);
  }
  else {
    s_shiftRegisters.clear(k_caseFanPin);
  }

  TRACE("Case fan shift");
  s_shiftRegisters.shift();
}

void System::Beep(int times, bool longBeep)
{
  for (int i = 0; i < times; i++) {
    TRACE("Beep set shift");
    s_shiftRegisters.set_shift(k_BeepPin);
    Delay(longBeep ? 200 : 100, "Beep on");

    TRACE("Beep clear shift");
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
    TRACE_F("Switch set: %d", pin);
    s_shiftRegisters.set(pin);
    m_switchState[index] = true;
  }
  else {
    TRACE_F("Switch clear: %d", pin);
    s_shiftRegisters.clear(pin);
    m_switchState[index] = false;
  }

  TRACE("Switch shift");
  s_shiftRegisters.shift();

  ReportSwitchStates();

  UpdateCaseFan();
}

void System::ToggleSwitch(int switchIndex)
{
  if (!m_switchState[switchIndex]) {
    TRACE_F("Toggle switch on: %d", switchIndex);
    SetSwitch(switchIndex, true);
  }
  else {
    TRACE_F("Toggle switch off: %d", switchIndex);
    SetSwitch(switchIndex, false);
  }
}

float System::ReadAdc(ADC &adc, ADS1115_MUX channel)
{
  if (!adc.ready) {
#if ADC_DEBUG
    TRACE_F("ADC not ready: %s", adc.name.c_str());
    TRACE("Retrying ADC init.");
#endif // ADC_DEBUG

    adc.ready = adc.ads.init();

    if (!adc.ready) {
#if ADC_DEBUG
      TRACE("ADC still not ready, giving up.");
#endif // ADC_DEBUG
      return k_unknown;
    }
  }

  // HACK: this keeps getting reset to default (possibly due to a power issue
  // when the relay switches), so force the volt range every time we're about to read.
  adc.ads.setVoltageRange_mV(ADS1115_RANGE_6144);

  adc.ads.setCompareChannels(channel);
  adc.ads.startSingleMeasurement();

  int times = 0;
  while (adc.ads.isBusy()) {
    Delay(ADC_RETRY_DELAY, "ADC busy wait");
    if (times++ > ADC_RETRY_MAX) {
#if ADC_DEBUG
      TRACE("ADC is busy, result unknown.");
#endif // ADC_DEBUG
      return k_unknown;
    }
  }

  return adc.ads.getResult_V(); // alternative: getResult_mV for Millivolt
}

void System::ReportInfo(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  TRACE_C(s_reportBuffer);
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
  TRACE_C(s_reportBuffer);

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

  TRACE_C(s_reportBuffer);
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

void System::ReportWindowOpenPercent() { Blynk.virtualWrite(V9, WindowOpenPercentExpected()); }

void System::ReportSystemInfo()
{
  // current time
  Blynk.virtualWrite(V10, Time().FormattedCurrentTime());

  // uptime
  Blynk.virtualWrite(V12, Time().FormattedUptime());

  // free heap
  int freeHeap = ESP.getFreeHeap();
  TRACE_F("Free heap: %d bytes", freeHeap);
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
  TRACE("First refresh");
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
    TRACE_F("Invalid refresh rate: %ds", m_refreshRate);
    return;
  }

  TRACE_F("New refresh rate: %ds", m_refreshRate);

  if (m_timerId != k_unknown) {
    TRACE_F("Deleting old timer: %d", m_timerId);
    s_refreshTimer.deleteTimer(m_timerId);
  }

  m_timerId = s_refreshTimer.setInterval(m_refreshRate * 1000L, refreshTimer);
  TRACE_F("New refresh timer: %d", m_timerId);
}

bool System::UpdateWeatherForecast()
{
  char uri[100];
  sprintf(uri, k_weatherUri, k_weatherLat, k_weatherLon, k_weatherApiKey);

  TRACE_F("Connecting to weather host: %s", k_weatherHost);
  HttpClient httpClient(s_wifiClient, k_weatherHost, 80);

  TRACE("Weather host get");
  httpClient.get(uri);

  int statusCode = httpClient.responseStatusCode();
  if (isnan(statusCode)) {
    TRACE("Weather host status is invalid");
    return false;
  }

  TRACE_F("Weather host status: %d", statusCode);
  if (statusCode != 200) {
    TRACE("Weather host error status");
    return false;
  }

  String response = httpClient.responseBody();
  int size = static_cast<int>(strlen(response.c_str()));
  TRACE_F("Weather host response length: %d", size);

  DeserializationError error = deserializeJson(s_weatherJson, response);
  if (error != DeserializationError::Ok) {
    TRACE_F("Weather data error: %s", error.c_str());
    return false;
  }

  TRACE("Deserialized weather JSON");
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

  TRACE_F("Weather forecast: code=%d, info='%s'", id, s_weatherInfo);

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
  bool caseFanOn = (s_switchOnCount > 0) || (!Power().BatteryPowerSource());
  CaseFan(caseFanOn);
}

void System::OnPowerSwitch()
{
  UpdateCaseFan();
  Blynk.virtualWrite(V28, Power().BatteryPowerSource());
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

bool System::BatterySensorReady() { return s_adc2.ready; }

float System::ReadBatteryVoltageSensorRaw() { return ReadAdc(s_adc2, k_batteryVoltagePin); }

float System::ReadBatteryCurrentSensorRaw() { return ReadAdc(s_adc2, k_batteryCurrentPin); }

float System::ReadLocalVoltageSensorRaw() { return ReadAdc(s_adc1, k_localVoltagePin); }

void System::PrintCommands() { TRACE("Commands\ns: status"); }

void System::PrintStatus()
{
  TRACE_F("Status\nBlynk: %s", Blynk.connected() ? "Connected" : "Disconnected");
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
  eg::s_instance->WindowOpenPercent(param.asInt());
  eg::s_instance->QueueCallback(&eg::System::UpdateWindowOpenPercent, "Window open percent");
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

BLYNK_WRITE(V34) { eg::s_instance->Power().BatteryVoltageSensorMin(param.asFloat()); }

BLYNK_WRITE(V35) { eg::s_instance->Power().BatteryVoltageSensorMax(param.asFloat()); }

BLYNK_WRITE(V36) { eg::s_instance->Power().BatteryVoltageOutputMin(param.asFloat()); }

BLYNK_WRITE(V37) { eg::s_instance->Power().BatteryVoltageOutputMax(param.asFloat()); }

BLYNK_WRITE(V38) { eg::s_instance->Power().BatteryCurrentSensorMin(param.asFloat()); }

BLYNK_WRITE(V39) { eg::s_instance->Power().BatteryCurrentSensorMax(param.asFloat()); }

BLYNK_WRITE(V40) { eg::s_instance->Power().BatteryCurrentOutputMin(param.asFloat()); }

BLYNK_WRITE(V41) { eg::s_instance->Power().BatteryCurrentOutputMax(param.asFloat()); }

BLYNK_WRITE(V44) { eg::s_instance->Power().BatteryVoltageSwitchOn(param.asFloat()); }

BLYNK_WRITE(V45) { eg::s_instance->Power().BatteryVoltageSwitchOff(param.asFloat()); }

BLYNK_WRITE(V47)
{
  eg::s_instance->Power().BatteryMode((embedded::greenhouse::BatteryModes)param.asInt());
}

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
