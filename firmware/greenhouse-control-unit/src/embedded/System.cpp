#include "System.h"

#include <Adafruit_SHT31.h>
#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp32.h>
#include <PCF8574.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <trace.h>

#include <cstdarg>

#include "Heating.h"
#include "ISystem.h"
#include "common.h"
#include "gh_config.h"
#include "radio.h"
#include "radio_common.h"

using namespace greenhouse;

namespace ng = greenhouse::native;
namespace eg = greenhouse::embedded;

namespace greenhouse {
namespace embedded {

#define SR_ON LOW
#define SR_OFF HIGH
#define LOOP_DELAY 10
#define SOIL_TEMP_FAIL_MAX 10
#define CASE_FAN_CURRENT_THRESHOLD 1
#define SHORT_BEEP_TIME 100
#define LONG_BEEP_TIME 400
#define BEEP_OFF_TIME 200
#define IO_PIN_BEEP_SW P1
#define IO_PIN_FAN_SW P0
#define NODE_SWITCH_0 P1
#define NODE_SWITCH_1 P2
#define NODE_RESET_DELAY 100
#define NODE_BOOT_DELAY 2000

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
const uint8_t k_localSystemIoAddress1 = 0x38;
const uint8_t k_externSwitchlIoAddress1 = 0x39;
const int k_loopFrequency = 1000;      // 1s
const int k_blynkFailuresMax = 300;
const int k_blynkRecoverTimeSec = 300; // 5m
const int k_serialWaitDelay = 1000;    // 1s
const int k_leftWindowNodeSwitch = 1;
const int k_rightWindowNodeSwitch = 2;

static System *s_instance = nullptr;
static PCF8574 s_localSystemIo1(k_localSystemIoAddress1);
static PCF8574 s_externalSwitchIo1(k_externSwitchlIoAddress1);
static char s_reportBuffer[200];
static BlynkTimer s_refreshTimer;
static Adafruit_SHT31 s_insideAirSensor;
static Adafruit_SHT31 s_outsideAirSensor;
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

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  TRACE_F(TRACE_DEBUG1, "Starting system: %s", BLYNK_DEVICE_NAME);

  TRACE(TRACE_DEBUG1, "Init I2C");
  if (!Wire.begin()) {
    TRACE(TRACE_DEBUG1, "I2C init failed");
    Restart();
  }

  InitIO();

  Beep(1, false);
  CaseFan(true); // on by default

  TRACE(TRACE_DEBUG1, "Init power");
  m_power.Embedded(*this);
  m_power.Native(*this);
  m_power.Setup();

  InitSensors();

  TRACE(TRACE_DEBUG1, "Init Blynk");
  Blynk.begin(k_auth, k_ssid, k_pass);

  TRACE(TRACE_DEBUG1, "Init time");
  m_time.Setup();

  radio::init(this);

  TRACE(TRACE_DEBUG1, "System ready");
  Blynk.virtualWrite(V52, "Ready");
  Beep(2, false);
  digitalWrite(LED_BUILTIN, HIGH);
}

void System::Loop()
{
  radio::update();

  if (millis() < (m_lastLoop + k_loopFrequency)) {
    return;
  }
  m_lastLoop = millis();

  UpdateSoilTemps(false);

  // run early so that refresh is queued on time.
  s_refreshTimer.run();

  // always run before actuator check
  ng::System::Loop();

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
    TRACE_F(TRACE_DEBUG1, "Blynk failed %d time(s)", m_blynkFailures);

    if (m_blynkFailures >= k_blynkFailuresMax) {
      TRACE(TRACE_DEBUG1, "Restarting, too many Blynk failures");
      Restart();
      return;
    }
  }
  else if (recoveredWithinTimeframe && (m_blynkFailures != 0)) {
    // no failures happened within last n - r seconds (n = now, r = recover
    // time)
    TRACE(TRACE_DEBUG1, "Blynk is back to normal, resetting fail count");
    m_blynkFailures = 0;
  }

  Power().Loop();

  if (SystemStarted()) {
    while (!m_callbackQueue.empty()) {
      Callback callback = m_callbackQueue.front();

      TRACE_F(TRACE_DEBUG1, "Callback function: %s", callback.m_name.c_str());

      m_callbackQueue.pop();

      if (callback.m_function != nullptr) {
        (this->*callback.m_function)();
      }
      else {
        TRACE(TRACE_DEBUG1, "Function pointer was null");
      }

      TRACE_F(TRACE_DEBUG1, "Callback queue remaining=%d", m_callbackQueue.size());
    }
  }

  if (m_queueOnSystemStarted) {
    m_queueOnSystemStarted = false;
    OnSystemStarted();
  }

  yield();
}

void System::InitIO()
{
  TRACE(TRACE_DEBUG1, "Init local system PCF8574 IO");

  s_localSystemIo1.pinMode(P0, OUTPUT);
  s_localSystemIo1.pinMode(P1, OUTPUT);
  s_localSystemIo1.pinMode(P2, OUTPUT);
  s_localSystemIo1.pinMode(P3, OUTPUT);
  s_localSystemIo1.pinMode(P4, OUTPUT);
  s_localSystemIo1.pinMode(P5, OUTPUT);
  s_localSystemIo1.pinMode(P6, OUTPUT);
  s_localSystemIo1.pinMode(P7, OUTPUT);

  if (!s_localSystemIo1.begin()) {
    TRACE(TRACE_DEBUG1, "Local system PCF8574 failed");
    Restart();
  }

  TRACE(TRACE_DEBUG1, "Init external switch PCF8574 IO");

  s_externalSwitchIo1.pinMode(P0, OUTPUT);
  s_externalSwitchIo1.pinMode(P1, OUTPUT);
  s_externalSwitchIo1.pinMode(P2, OUTPUT);
  s_externalSwitchIo1.pinMode(P3, OUTPUT);
  s_externalSwitchIo1.pinMode(P4, OUTPUT);
  s_externalSwitchIo1.pinMode(P5, OUTPUT);
  s_externalSwitchIo1.pinMode(P6, OUTPUT);
  s_externalSwitchIo1.pinMode(P7, OUTPUT);

  if (!s_externalSwitchIo1.begin()) {
    TRACE(TRACE_DEBUG1, "External switch PCF8574 failed");
    Restart();
  }
}

void System::InitSensors()
{
  TRACE(TRACE_DEBUG1, "Init sensors");

  if (!s_insideAirSensor.begin(k_insideAirSensorAddress)) {
    ReportWarning("Inside air sensor not found");
  }

#if OUTSIDE_SHT31_EN
  if (!s_outsideAirSensor.begin(k_outsideAirSensorAddress)) {
    ReportWarning("Outside air sensor not found");
  }
#endif // OUTSIDE_SHT31_EN
}

void System::Refresh(bool first)
{
  TRACE(TRACE_DEBUG1, "Refreshing (embedded)");

  Time().Refresh();

  FlashLed(k_ledRefresh);

  ng::System::Refresh(first);

  m_power.MeasureCurrent();

  Blynk.virtualWrite(V28, Power().Source() == k_powerSourceBattery);
  Blynk.virtualWrite(V29, Power().BatteryVoltageSensor());
  Blynk.virtualWrite(V30, Power().BatteryVoltageOutput());
  Blynk.virtualWrite(V42, Power().BatteryCurrentSensor());
  Blynk.virtualWrite(V43, Power().BatteryCurrentOutput());
  Blynk.virtualWrite(V55, Heating().WaterHeaterIsOn());
  Blynk.virtualWrite(V56, Heating().SoilHeatingIsOn());
  Blynk.virtualWrite(V59, Heating().AirHeatingIsOn());

  UpdateSoilTemps(true);
  radio::getSoilTempsAsync();

  TRACE(TRACE_DEBUG1, "Refresh done (embedded)");
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
#if SENSORS_EN

  if (TestMode()) {
    TRACE(TRACE_DEBUG1, "Test mode: Skipping sensor read");
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

#if OUTSIDE_SHT31_EN
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
#endif // OUTSIDE_SHT31_EN

  return failures == 0;

#else

  return true;

#endif // SENSORS_EN
}

void System::UpdateSoilTemps(bool force)
{
  bool changed = m_soilTemperature != radio::getSoilTempsResult();
  if (changed) {
    TRACE(TRACE_DEBUG1, "Soil temps changed");
  }

  if (changed || force) {
    m_soilTemperature = radio::getSoilTempsResult();
    if (m_soilTemperature != k_unknown) {
      TRACE_F(TRACE_DEBUG1, "Soil temps: %.2f°C", m_soilTemperature);
    }
    else {
      TRACE(TRACE_DEBUG1, "Soil temps unknown");
    }
    Blynk.virtualWrite(V11, SoilTemperature());
  }
}

void System::RunWindowActuator(bool extend, float delta)
{
  int runtimeSec = ceilf(WindowActuatorRuntimeSec() * delta);
  TRACE_F(
    TRACE_DEBUG1,
    "Running window actuator: delta=%.2f, max=%.2fs, runtime=%.2fs",
    delta,
    WindowActuatorRuntimeSec(),
    runtimeSec);

  if (extend) {
    radio::windowActuatorRunAll(radio::k_windowExtend, runtimeSec);
  }
  else {
    radio::windowActuatorRunAll(radio::k_windowRetract, runtimeSec);
  }
}

void System::Delay(unsigned long ms, const char *reason)
{
#if TRACE_DELAY
  TRACE_F(TRACE_DEBUG1, "Delay %dms (%s)", (int)ms, reason);
#endif // TRACE_DELAY
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
  TRACE_F(TRACE_DEBUG1, "Case fan %s", on ? "on" : "off");
  s_localSystemIo1.digitalWrite(IO_PIN_FAN_SW, on);
}

void System::Beep(int times, bool longBeep)
{
  TRACE_F(TRACE_DEBUG1, "Beep %d times (%s)", times, longBeep ? "long" : "short");
  for (int i = 0; i < times; i++) {
    s_localSystemIo1.digitalWrite(IO_PIN_BEEP_SW, LOW);
    Delay(longBeep ? LONG_BEEP_TIME : SHORT_BEEP_TIME, "Beep on");

    s_localSystemIo1.digitalWrite(IO_PIN_BEEP_SW, HIGH);
    Delay(BEEP_OFF_TIME, "Beep off");
  }
}

void System::ReportInfo(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  TRACE_C(TRACE_DEBUG1, s_reportBuffer);
  Blynk.logEvent("info", s_reportBuffer);
}

void System::ReportWarning(const char *format, ...)
{
#if BLYNK_WARNINGS
  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  // still log even if k_reportWarnings is off (the intent of k_reportWarnings
  // is to reduce noise on the blynk app during testing)
  TRACE_C(TRACE_DEBUG1, s_reportBuffer);

  Blynk.logEvent("warning", s_reportBuffer);
#endif // BLYNK_WARNINGS
}

void System::ReportCritical(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vsprintf(s_reportBuffer, format, args);
  va_end(args);

  TRACE_C(TRACE_DEBUG1, s_reportBuffer);
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
  TRACE_F(TRACE_DEBUG1, "Free heap: %d bytes", freeHeap);
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

  ResetNodes();
  ResetRelay();

  // run first refresh (instead of waiting for the 1st refresh timer).
  // we run the 1st refresh here instead of when the timer is created,
  // because when we setup the timer for the first time, we may not
  // have all of the correct initial values.
  TRACE(TRACE_DEBUG1, "First refresh");
  Refresh(true);

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
    TRACE_F(TRACE_DEBUG1, "Invalid refresh rate: %ds", m_refreshRate);
    return;
  }

  TRACE_F(TRACE_DEBUG1, "New refresh rate: %ds", m_refreshRate);

  if (m_timerId != k_unknown) {
    TRACE_F(TRACE_DEBUG1, "Deleting old timer: %d", m_timerId);
    s_refreshTimer.deleteTimer(m_timerId);
  }

  m_timerId = s_refreshTimer.setInterval(m_refreshRate * 1000L, refreshTimer);
  TRACE_F(TRACE_DEBUG1, "New refresh timer: %d", m_timerId);
}

bool System::UpdateWeatherForecast()
{
#if WEATHER_EN

  if (TestMode()) {
    return true;
  }

  char uri[100];
  sprintf(uri, k_weatherUri, k_weatherLat, k_weatherLon, k_weatherApiKey);

  TRACE_F(TRACE_DEBUG1, "Connecting to weather host: %s", k_weatherHost);
  HttpClient httpClient(s_wifiClient, k_weatherHost, 80);

  TRACE(TRACE_DEBUG1, "Weather host get");
  httpClient.get(uri);

  int statusCode = httpClient.responseStatusCode();
  if (isnan(statusCode)) {
    TRACE(TRACE_DEBUG1, "Weather host status is invalid");
    return false;
  }

  TRACE_F(TRACE_DEBUG1, "Weather host status: %d", statusCode);
  if (statusCode != 200) {
    TRACE(TRACE_DEBUG1, "Weather host error status");
    return false;
  }

  String response = httpClient.responseBody();
  int size = static_cast<int>(strlen(response.c_str()));
  TRACE_F(TRACE_DEBUG1, "Weather host response length: %d", size);

  DeserializationError error = deserializeJson(s_weatherJson, response);
  if (error != DeserializationError::Ok) {
    TRACE_F(TRACE_DEBUG1, "Weather data error: %s", error.c_str());
    return false;
  }

  TRACE(TRACE_DEBUG1, "Deserialized weather JSON");
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

  TRACE_F(TRACE_DEBUG1, "Weather forecast: code=%d, info='%s'", id, s_weatherInfo);

  WeatherCode(id);
  WeatherInfo(s_weatherInfo);
  ReportWeather();
  return true;

#else

  return true;

#endif // WEATHER_EN
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
  bool onPsu = Power().Source() != PowerSource::k_powerSourceBattery;

  bool highCurrent = Power().BatteryCurrentOutput() > CASE_FAN_CURRENT_THRESHOLD;

  bool on = onPsu || highCurrent;
  CaseFan(on);

  TRACE_F(
    TRACE_DEBUG1,
    "Case fan: %s (PSU=%s, HC=%s)", //
    BOOL_FS(on),
    BOOL_FS(onPsu),
    BOOL_FS(highCurrent));
}

void System::OnPowerSwitch()
{
  UpdateCaseFan();
  Blynk.virtualWrite(V28, Power().Source() == k_powerSourceBattery);
}

void System::OnBatteryCurrentChange() { UpdateCaseFan(); }

void System::ResetNodes()
{
  TRACE(TRACE_DEBUG1, "Resetting nodes");
  s_externalSwitchIo1.digitalWrite(NODE_SWITCH_0, HIGH);
  s_externalSwitchIo1.digitalWrite(NODE_SWITCH_1, HIGH);

  Delay(NODE_RESET_DELAY, "Node reset");

  s_externalSwitchIo1.digitalWrite(NODE_SWITCH_0, LOW);
  s_externalSwitchIo1.digitalWrite(NODE_SWITCH_1, LOW);

  Delay(NODE_BOOT_DELAY, "Node boot");
}

void System::ResetRelay() { radio::resetRelay(); }

void System::WriteOnboardIO(uint8_t pin, uint8_t value)
{
  s_localSystemIo1.digitalWrite(pin, value);
}

void System::PrintCommands() { TRACE(TRACE_DEBUG1, "Commands\ns: status"); }

void System::PrintStatus()
{
  TRACE_F(TRACE_DEBUG1, "Status\nBlynk: %s", Blynk.connected() ? "Connected" : "Disconnected");
}

void System::QueueCallback(CallbackFunction f, std::string name)
{
  m_callbackQueue.push(Callback(f, name));
}

void System::WindowSpeedUpdate()
{
  radio::windowActuatorSetup(WindowSpeedLeft(), WindowSpeedRight());
}

void System::ReportPumpSwitch(bool pumpOn) { Blynk.virtualWrite(V80, pumpOn); }

void System::ReportPumpStatus(const char *message) { Blynk.virtualWrite(V81, message); }

void System::SwitchLights(bool on) { TRACE(TRACE_DEBUG1, "Lights not implemented"); }

} // namespace embedded
} // namespace greenhouse

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

BLYNK_WRITE(V44) { eg::s_instance->Power().BatteryVoltageSwitchOn(param.asFloat()); }

BLYNK_WRITE(V45) { eg::s_instance->Power().BatteryVoltageSwitchOff(param.asFloat()); }

BLYNK_WRITE(V47) { eg::s_instance->Power().Mode((greenhouse::embedded::PowerMode)param.asInt()); }

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
    eg::s_instance->ReportPumpStatus("Manually switching on");
    Blynk.logEvent("info", F("Manually switching pump on."));
  }
  else {
    eg::s_instance->ReportPumpStatus("Manually switching off");
    Blynk.logEvent("info", F("Manually switching pump off."));
  }
  radio::pumpSwitch(pumpOn);
}

BLYNK_WRITE(V82)
{
  const bool tankFull = static_cast<bool>(param.asInt());
  if (tankFull) {
    eg::s_instance->ReportPumpStatus("Auto switching off");
    Blynk.logEvent("info", F("Tank is full, auto switching pump off."));
  }
  else {
    eg::s_instance->ReportPumpStatus("Auto switching on");
    Blynk.logEvent("info", F("Tank is not full, auto switching pump on."));
  }

  // tank not full? switch pump on
  radio::pumpSwitch(!tankFull);
}
