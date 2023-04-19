#include "System.h"

#include <cstdarg>

#include "Heating.h"
#include "ISystem.h"
#include "common.h"
#include "gh_config.h"

#include <Adafruit_SHT31.h>
#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp32.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <PCF8574.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>

using namespace common;

namespace ng = native::greenhouse;
namespace eg = embedded::greenhouse;

namespace embedded {
namespace greenhouse {

#define SR_ON LOW
#define SR_OFF HIGH

#define ADC_BUSY_MAX 10
#define ADC_BUSY_DELAY 1
#define ADC_RETRY_MAX 5
#define ADC_RETRY_DELAY 1000
#define LOOP_DELAY 10
#define SOIL_TEMP_FAIL_MAX 10

struct ADC {
  String name;
  ADS1115_WE ads;
  bool ready = false;
};

#define IO_PIN_BEEP_SW P2
#define IO_PIN_FAN_SW P3

#define EXT_IO_PIN_LIGHT P1

// external adc pins
const ADS1115_MUX k_batteryVoltagePin = ADS1115_COMP_0_GND;
const ADS1115_MUX k_batteryCurrentPin = ADS1115_COMP_1_GND;

const bool k_enableLed = false;
const int k_ledFlashDelay = 30; // ms
const int k_soilProbeIndex = 0;
const int k_waterProbeIndex = 1;
const uint8_t k_insideAirSensorAddress = 0x44;
const uint8_t k_outsideAirSensorAddress = 0x45;
const int k_externalAdcAddress = 0x49;
const float k_weatherLat = 54.203;
const float k_weatherLon = -4.408;
const char *k_weatherApiKey = "e8444a70abfc2b472d43537730750892";
const char *k_weatherHost = "api.openweathermap.org";
const char *k_weatherUri = "/data/2.5/weather?lat=%.3f&lon=%.3f&units=metric&appid=%s";
const uint8_t k_onboardIoAddress1 = 0x20;
const uint8_t k_externalIoAddress1 = 0x21;
const int k_loopFrequency = 1000; // 1s
const int k_blynkFailuresMax = 300;
const int k_blynkRecoverTimeSec = 300; // 5m
const int k_serialWaitDelay = 1000;    // 1s
const int k_leftWindowNodeSwitch = 1;
const int k_rightWindowNodeSwitch = 2;

static System *s_instance = nullptr;
static PCF8574 s_onboardIo1(k_onboardIoAddress1);
static PCF8574 s_externalIo1(k_externalIoAddress1);
static char s_reportBuffer[200];
static BlynkTimer s_refreshTimer;
static Adafruit_SHT31 s_insideAirSensor;
static Adafruit_SHT31 s_outsideAirSensor;
static ADC s_externalAdc;
static WiFiClient s_wifiClient;
static char s_weatherInfo[50];
static DynamicJsonDocument s_weatherJson(2048);

// free-function declarations

void refreshTimer() { eg::s_instance->QueueCallback(&eg::System::Refresh, "Refresh (timer)"); }

// member functions

System &System::Instance() { return *s_instance; }

void System::Instance(System &ga) { s_instance = &ga; }

System::System() :
  m_heating(),
  m_power(),
  m_insideAirTemperature(k_unknown),
  m_insideAirHumidity(k_unknown),
  m_outsideAirTemperature(k_unknown),
  m_outsideAirHumidity(k_unknown),
  m_soilTemperature(k_unknown),
  m_soilTemperatureFailures(0),
  m_waterTemperature(k_unknown),
  m_timerId(k_unknown),
  m_led(LOW),
  m_fakeInsideHumidity(k_unknown),
  m_fakeSoilTemperature(k_unknown),
  m_refreshBusy(false),
  m_insideHumidityWarningSent(false),
  m_blynkFailures(0),
  m_lastBlynkFailure(0),
  m_refreshRate(k_unknown),
  m_queueOnSystemStarted(false),
  m_lastLoop(0),
  m_windowSpeedLeft(k_unknown),
  m_windowSpeedRight(k_unknown)
{
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

  TRACE("Init I2C");
  if (!Wire.begin()) {
    TRACE("I2C init failed");
    Restart();
  }

  InitIO();

  Beep(1, false);
  CaseFan(true); // on by default

  TRACE("Init power");
  m_power.Embedded(*this);
  m_power.Native(*this);
  m_power.Setup();

  InitSensors();
  InitExternalADC();

  TRACE("Init Blynk");
  Blynk.begin(k_auth, k_ssid, k_pass);

  TRACE("Init time");
  m_time.Setup();

#if RADIO_EN
  TRACE("Init radio");
  m_radio.Init(this);
#endif // RADIO_EN

  m_pumpRadio.Init(this);

  TRACE("System ready");
  Blynk.virtualWrite(V52, "Ready");
  Beep(2, false);
}

void System::Loop()
{
  if (millis() < (m_lastLoop + k_loopFrequency)) {
    delay(LOOP_DELAY);
    return;
  }
  m_lastLoop = millis();

  // run early so that refresh is queued on time.
  s_refreshTimer.run();

#if RADIO_EN
  // run first for keep alive
  m_radio.Update();
#endif // RADIO_EN

  // always run before actuator check
  ng::System::Loop();

  m_pumpRadio.Update();

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

void System::InitIO()
{
  TRACE("Init onboard PCF8574 IO");

  s_onboardIo1.pinMode(P0, OUTPUT);
  s_onboardIo1.pinMode(P1, OUTPUT);
  s_onboardIo1.pinMode(P2, OUTPUT);
  s_onboardIo1.pinMode(P3, OUTPUT);
  s_onboardIo1.pinMode(P4, OUTPUT);
  s_onboardIo1.pinMode(P5, OUTPUT);
  s_onboardIo1.pinMode(P6, OUTPUT);
  s_onboardIo1.pinMode(P7, OUTPUT);

  if (!s_onboardIo1.begin()) {
    TRACE("Onboard PCF8574 failed");
    Restart();
  }

  TRACE("Init external PCF8574 IO");

  s_externalIo1.pinMode(P0, OUTPUT);
  s_externalIo1.pinMode(P1, OUTPUT);
  s_externalIo1.pinMode(P2, OUTPUT);
  s_externalIo1.pinMode(P3, OUTPUT);
  s_externalIo1.pinMode(P4, OUTPUT);
  s_externalIo1.pinMode(P5, OUTPUT);
  s_externalIo1.pinMode(P6, OUTPUT);
  s_externalIo1.pinMode(P7, OUTPUT);

  if (!s_externalIo1.begin()) {
    TRACE("External PCF8574 failed");
    Restart();
  }
}

void System::InitSensors()
{
  TRACE("Init sensors");

  if (!s_insideAirSensor.begin(k_insideAirSensorAddress)) {
    ReportWarning("Inside air sensor not found");
  }

  if (!s_outsideAirSensor.begin(k_outsideAirSensorAddress)) {
    ReportWarning("Outside air sensor not found");
  }
}

void System::InitExternalADC()
{
  TRACE("Init external ADC");

  s_externalAdc.name = "Battery";
  s_externalAdc.ads = ADS1115_WE(k_externalAdcAddress);
  s_externalAdc.ready = s_externalAdc.ads.init();
  if (!s_externalAdc.ready) {
    ReportWarning("ADC init failed: %s", s_externalAdc.name.c_str());
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

  Blynk.virtualWrite(V28, Power().Source() == k_powerSourceBattery);
  Blynk.virtualWrite(V29, Power().BatteryVoltageSensor());
  Blynk.virtualWrite(V30, Power().BatteryVoltageOutput());
  Blynk.virtualWrite(V42, Power().BatteryCurrentSensor());
  Blynk.virtualWrite(V43, Power().BatteryCurrentOutput());
  Blynk.virtualWrite(V55, Heating().WaterHeaterIsOn());
  Blynk.virtualWrite(V56, Heating().SoilHeatingIsOn());
  Blynk.virtualWrite(V59, Heating().AirHeatingIsOn());

#if RADIO_EN
  Blynk.virtualWrite(V77, m_radio.DebugInfo());
#endif // RADIO_EN

  UpdateSoilTemp();

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

  return failures == 0;
}

void System::UpdateSoilTemp()
{
#if RADIO_EN
  TRACE("Reading soil temperatures");
  radio::Node &rightWindow = m_radio.Node(radio::k_nodeRightWindow);
  const int tempDevs = rightWindow.GetTempDevs();

  TRACE_F("Soil temperature devices: %d", tempDevs);
  float tempSum = 0;
  int tempValues = 0;
  for (int i = 0; i < tempDevs; i++) {
    const float t = rightWindow.GetTemp(i);
    if (t != k_unknown) {
      tempSum += t;
      tempValues++;
      TRACE_F("Soil temperature sensor %d: %.2f", i, t);
    }
    else {
      TRACE_F("Soil temperature sensor %d: unknown", i);
    }
  }

  if (tempValues > 0) {
    m_soilTemperatureFailures = 0;
    m_soilTemperature = tempSum / tempValues;
    TRACE_F("Soil temperature average: %.2f", m_soilTemperature);
  }
  else if (++m_soilTemperatureFailures > SOIL_TEMP_FAIL_MAX) {
    // only report -1 if there are a number of failures
    m_soilTemperatureFailures = 0;
    m_soilTemperature = k_unknown;
    TRACE("Soil temperatures unknown");
  }
#else
  m_soilTemperature = k_unknown;
#endif // RADIO_EN

  Blynk.virtualWrite(V11, SoilTemperature());
}

void System::RunWindowActuator(bool extend, float delta)
{
  int runtimeSec = ceilf(WindowActuatorRuntimeSec() * delta);
  TRACE_F(
    "Running window actuator: delta=%.2f, max=%.2fs, runtime=%.2fs",
    delta,
    WindowActuatorRuntimeSec(),
    runtimeSec);

#if RADIO_EN
  if (extend) {
    m_radio.MotorRunAll(radio::k_windowExtend, runtimeSec);
  }
  else {
    m_radio.MotorRunAll(radio::k_windowRetract, runtimeSec);
  }
#endif // RADIO_EN
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

  Blynk.disconnect();
  esp_restart();
}

void System::CaseFan(bool on)
{
  TRACE_F("Case fan %s", on ? "on" : "off");
  s_onboardIo1.digitalWrite(IO_PIN_FAN_SW, on);
}

void System::Beep(int times, bool longBeep)
{
  TRACE_F("Beep %d times (%s)", times, longBeep ? "long" : "short");
  for (int i = 0; i < times; i++) {
    s_onboardIo1.digitalWrite(IO_PIN_BEEP_SW, LOW);
    Delay(longBeep ? 200 : 100, "Beep on");

    s_onboardIo1.digitalWrite(IO_PIN_BEEP_SW, HIGH);
    Delay(200, "Beep off");
  }
}

float System::ReadAdc(ADC &adc, ADS1115_MUX channel)
{
#if ADC_DEBUG
  TRACE_F("Reading ADC: %s", adc.name.c_str());
#endif

  // TODO: random init fal probably caused by bad hardware;
  // I2C line to external device is probably acting as an antenna and picking up noise.
  // seems to be caused by load on the battery. 
  for (int i = 0; i < ADC_RETRY_MAX; i++) {

    TRACE_F("ADC: Attempt %d of %d", i + 1, ADC_RETRY_MAX);

    if (!adc.ready) {
#if ADC_DEBUG
      TRACE("ADC: Re-init");
#endif // ADC_DEBUG

      adc.ready = adc.ads.init();
      if (adc.ready) {
        TRACE("ADC: Ready again");
      }
      else {
#if ADC_DEBUG
        TRACE("ADC: Re-init failed, retrying");
#endif // ADC_DEBUG
        Delay(ADC_RETRY_DELAY, "ADC retry wait");
        yield();
        continue;
      }
    }

    // HACK: this keeps getting reset to default (possibly due to a power issue
    // when the relay switches), so force the volt range every time we're about to read.
    adc.ads.setVoltageRange_mV(ADS1115_RANGE_2048);

    adc.ads.setCompareChannels(channel);
    adc.ads.startSingleMeasurement();

    int times = 0;
    while (adc.ads.isBusy()) {
      if (times++ > ADC_BUSY_MAX) {

        // assume ADC not ready if failed many times. should cause re-init.
        adc.ready = false;
#if ADC_DEBUG
        TRACE_F("ADC: Still busy after %d times", times);
#endif // ADC_DEBUG
        break;
      }
      else {
        Delay(ADC_BUSY_DELAY, "ADC busy wait");
        TRACE_F("ADC: Busy wait %d of %d", times, ADC_BUSY_MAX);
      }
    }

    // if assumed not ready, wait longer and start new comparison
    if (!adc.ready) {
#if ADC_DEBUG
      TRACE_F("ADC: Assumed not ready, retrying", times);
#endif // ADC_DEBUG
      Delay(ADC_RETRY_DELAY, "ADC retry wait");
      yield();
      continue;
    }

    float f = adc.ads.getResult_V(); // alternative: getResult_mV for Millivolt

#if ADC_DEBUG
    TRACE_F("ADC: Got result: %.2fV", f);
#endif

    return f;
  }

  return k_unknown;
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
}

void System::OnLastWrite()
{
  if (!SystemStarted()) {
    m_queueOnSystemStarted = true;
  }
}

void System::OnSystemStarted()
{
  // Power().InitPowerSource();

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
  if (TestMode()) {
    return true;
  }

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

void System::UpdateCaseFan()
{
  // turn case fan on when PSU is in use
  bool caseFanOn = Power().Source() != PowerSource::k_powerSourceBattery;
  CaseFan(caseFanOn);
}

void System::OnPowerSwitch()
{
  UpdateCaseFan();
  Blynk.virtualWrite(V28, Power().Source() == k_powerSourceBattery);
}

void System::WriteOnboardIO(uint8_t pin, uint8_t value) { s_onboardIo1.digitalWrite(pin, value); }

bool System::BatterySensorReady() { return s_externalAdc.ready; }

float System::ReadBatteryVoltageSensorRaw() { return ReadAdc(s_externalAdc, k_batteryVoltagePin); }

float System::ReadBatteryCurrentSensorRaw() { return ReadAdc(s_externalAdc, k_batteryCurrentPin); }

void System::PrintCommands() { TRACE("Commands\ns: status"); }

void System::PrintStatus()
{
  TRACE_F("Status\nBlynk: %s", Blynk.connected() ? "Connected" : "Disconnected");
}

void System::QueueCallback(CallbackFunction f, std::string name)
{
  m_callbackQueue.push(Callback(f, name));
}

void System::WindowSpeedUpdate()
{
#if RADIO_EN
  radio::Node &left = m_radio.Node(radio::k_nodeLeftWindow);
  radio::Node &right = m_radio.Node(radio::k_nodeRightWindow);
  left.MotorSpeed(WindowSpeedLeft());
  right.MotorSpeed(WindowSpeedRight());
#endif // RADIO_EN
}

void System::LowerPumpOn(bool pumpOn) { Blynk.virtualWrite(V80, pumpOn); }

void System::LowerPumpStatus(const char *message) { Blynk.virtualWrite(V81, message); }

void System::SwitchLights(bool on)
{
  s_externalIo1.digitalWrite(EXT_IO_PIN_LIGHT, on ? LOW : HIGH);
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
    V78,
    V79,
    V80,
    V82,
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

BLYNK_WRITE(V17) { eg::s_instance->SwitchLights(param.asInt() == 1); }

BLYNK_WRITE(V18) { eg::s_instance->FakeSoilTemperature(param.asFloat()); }

BLYNK_WRITE(V23) { eg::s_instance->FakeInsideHumidity(param.asFloat()); }

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

BLYNK_WRITE(V47) { eg::s_instance->Power().Mode((embedded::greenhouse::PowerMode)param.asInt()); }

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

BLYNK_WRITE(V73) { eg::s_instance->Heating().Enabled(param.asInt() == 1); }

BLYNK_WRITE(V74) { eg::s_instance->WindowAdjustTimeframeSec(param.asInt()); }

BLYNK_WRITE(V75) { eg::s_instance->FakeWeatherCode(param.asInt()); }

BLYNK_WRITE(V76)
{
  if (param.asInt()) {
    eg::s_instance->QueueCallback(&eg::System::WindowFullClose, "Window full close");
  }
}

// used only as the last value
BLYNK_WRITE(V127) { eg::s_instance->OnLastWrite(); }

BLYNK_WRITE(V78)
{
  eg::s_instance->WindowSpeedLeft(param.asInt());
  eg::s_instance->QueueCallback(&eg::System::WindowSpeedUpdate, "Window speed update");
}

BLYNK_WRITE(V79)
{
  eg::s_instance->WindowSpeedRight(param.asInt());
  eg::s_instance->QueueCallback(&eg::System::WindowSpeedUpdate, "Window speed update");
}

BLYNK_WRITE(V80)
{
  const bool pumpOn = static_cast<bool>(param.asInt());
  if (pumpOn) {
    Blynk.logEvent("info", F("Manually switching pump on."));
  }
  else {
    Blynk.logEvent("info", F("Manually switching pump off."));
  }
  eg::s_instance->PumpRadio().SwitchPump(pumpOn);
}

BLYNK_WRITE(V82)
{
  const bool tankFull = static_cast<bool>(param.asInt());
  if (tankFull) {
    Blynk.logEvent("info", F("Tank is full, auto switching pump off."));
  }
  else {
    Blynk.logEvent("info", F("Tank is not full, auto switching pump on."));
  }
  // tank not full? switch pump on
  eg::s_instance->PumpRadio().SwitchPump(!tankFull);
}
