#include "GreenhouseArduino.h"

#include <cstdarg>

#include "ho_config.h"

#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <OneWire.h>
#include <WiFiUdp.h>

// 8266 free pins: D4 (led), A0
// https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
const int k_actuatorPinEnA = 4;  // GPIO4  = D2
const int k_actuatorPinIn1 = 0;  // GPIO0  = D3 (connected to flash)
const int k_actuatorPinIn2 = 12; // GPIO12 = D6
const int k_insideDhtPin = 14;   // GPIO14 = D5
const int k_outsideDhtPin = 5;   // GPIO5 = D1
const int k_oneWireBusPin = 13;  // GPIO13 = D7
const int k_moisturePin = 0;     // ADC0

const int k_actuatorSpeed = 255;
const int k_actuatorRuntimeSec = 12;
const int k_ledFlashDelay = 30; // ms
const int k_soilProbeIndex = 0;

static char reportBuffer[80];

DHT insideDht(k_insideDhtPin, DHT11);
DHT outsideDht(k_outsideDhtPin, DHT21);
OneWire oneWire(k_oneWireBusPin);
DallasTemperature sensors(&oneWire);

BlynkTimer timer;
WiFiUDP ntpUdp;
NTPClient timeClient(ntpUdp);

GreenhouseArduino *s_instance = nullptr;

GreenhouseArduino &GreenhouseArduino::Instance() { return *s_instance; }

void GreenhouseArduino::Instance(GreenhouseArduino &ga) { s_instance = &ga; }

void refreshTimer()
{
  s_instance->TraceFlash(F("Refresh timer"));
  s_instance->Refresh();
}

GreenhouseArduino::GreenhouseArduino() :
  m_log(),
  m_insideTemperature(k_unknown),
  m_insideHumidity(k_unknown),
  m_outsideTemperature(k_unknown),
  m_outsideHumidity(k_unknown),
  m_timerId(k_unknown),
  m_led(LOW),
  m_fakeSoilTemperature(k_unknown),
  m_refreshBusy(false),
  m_soilMoisture(k_unknown),
  m_fakeSoilMoisture(k_unknown),
  m_insideHumidityWarningSent(false),
  m_soilMoistureWarningSent(false)
{
}

void GreenhouseArduino::Setup()
{
  Greenhouse::Setup();

  Serial.begin(9600);

  // wait for serial to connect before first trace
  // TODO: test if this is really actually needed
  delay(1000);
  Log().Trace("\n\n%s: Starting system", BLYNK_DEVICE_NAME);

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(k_actuatorPinEnA, OUTPUT);
  pinMode(k_actuatorPinIn1, OUTPUT);
  pinMode(k_actuatorPinIn2, OUTPUT);

  analogWrite(k_actuatorPinEnA, k_actuatorSpeed);

  Blynk.begin(k_auth, k_ssid, k_pass);
  insideDht.begin();
  outsideDht.begin();
  sensors.begin();
}

void GreenhouseArduino::Loop()
{
  Greenhouse::Loop();

  timeClient.update();
  Blynk.run();
  timer.run();
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

  m_insideTemperature = insideDht.readTemperature();
  if (isnan(m_insideTemperature)) {
    m_insideTemperature = k_unknown;
    return false;
  }

  m_insideHumidity = insideDht.readHumidity();
  if (isnan(m_insideHumidity)) {
    m_insideHumidity = k_unknown;
    return false;
  }

  m_outsideTemperature = outsideDht.readTemperature();
  if (isnan(m_outsideTemperature)) {
    m_outsideTemperature = k_unknown;
    return false;
  }

  m_outsideHumidity = outsideDht.readHumidity();
  if (isnan(m_outsideHumidity)) {
    m_outsideHumidity = k_unknown;
    return false;
  }

  sensors.requestTemperatures();
  m_soilTemperature = sensors.getTempCByIndex(k_soilProbeIndex);
  if (isnan(m_soilTemperature)) {
    m_soilTemperature = k_unknown;
    return false;
  }

  int moistureAnalog = analogRead(k_moisturePin);
  m_soilMoisture = CalculateMoisture(moistureAnalog);

  return true;
}

void GreenhouseArduino::SystemDigitalWrite(int pin, int val) { digitalWrite(pin, val); }

void GreenhouseArduino::SystemDelay(unsigned long ms) { delay(ms); }

void GreenhouseArduino::CloseWindow(float delta)
{
  TraceFlash(F("Closing window..."));
  Log().Trace("Delta: %.2f", delta);

  Greenhouse::CloseWindow(delta);

  SystemDigitalWrite(k_actuatorPinIn1, LOW);
  SystemDigitalWrite(k_actuatorPinIn2, HIGH);

  int runtime = (k_actuatorRuntimeSec * 1000) * delta;
  Log().Trace("Close runtime: %dms", runtime);
  SystemDelay(runtime);

  SystemDigitalWrite(k_actuatorPinIn1, LOW);
  SystemDigitalWrite(k_actuatorPinIn2, LOW);

  float percent = delta * 100;
  Log().Trace("Window closed %.1f%%", percent);
}

void GreenhouseArduino::OpenWindow(float delta)
{
  TraceFlash(F("Opening window..."));
  Log().Trace("Delta: %.2f", delta);

  Greenhouse::OpenWindow(delta);

  SystemDigitalWrite(k_actuatorPinIn1, HIGH);
  SystemDigitalWrite(k_actuatorPinIn2, LOW);

  int runtime = (k_actuatorRuntimeSec * 1000) * delta;
  Log().Trace("Open runtime: %dms", runtime);
  SystemDelay(runtime);

  SystemDigitalWrite(k_actuatorPinIn1, LOW);
  SystemDigitalWrite(k_actuatorPinIn2, LOW);

  float percent = delta * 100;
  Log().Trace("Window opened %.1f%%", percent);
}

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

int GreenhouseArduino::CurrentHour() const { return timeClient.getHours(); }

void GreenhouseArduino::ReportInfo(const char *format, ...)
{
  FlashLed(k_ledSend);

  va_list args;
  va_start(args, format);
  vsprintf(reportBuffer, format, args);
  va_end(args);

  Blynk.logEvent("log_info", reportBuffer);
}

void GreenhouseArduino::ReportWarning(const char *format, ...)
{
  FlashLed(k_ledSend);

  va_list args;
  va_start(args, format);
  vsprintf(reportBuffer, format, args);
  va_end(args);

  Blynk.logEvent("log_warning", reportBuffer);
}

void GreenhouseArduino::ReportCritical(const char *format, ...)
{
  FlashLed(k_ledSend);

  va_list args;
  va_start(args, format);
  vsprintf(reportBuffer, format, args);
  va_end(args);

  Blynk.logEvent("log_critical", reportBuffer);
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
  Blynk.virtualWrite(V10, timeClient.getFormattedTime() + " UTC");

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

  if (!m_insideHumidityWarningSent && (m_insideHumidity >= IndoorHumidityWarning())) {
    ReportWarning("Inside humidity high (%d%%)", m_insideHumidity);
    m_insideHumidityWarningSent = true;
  }
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

void GreenhouseArduino::HandleLastWrite()
{
  if (m_lastWriteDone) {
    return;
  }

  m_lastWriteDone = true;

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

  if (m_timerId != -1) {
    Log().Trace("Deleting old timer: %d", m_timerId);
    timer.deleteTimer(m_timerId);
  }

  m_timerId = timer.setInterval(refreshRate * 1000L, refreshTimer);
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

void GreenhouseArduino::HandleFakeSoilTemperature(float fakeSoilTemperature)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle fake soil temperature: %.2fC", fakeSoilTemperature);

  m_fakeSoilTemperature = fakeSoilTemperature;
}

void GreenhouseArduino::HandleFakeSoilMoisture(float fakeSoilMoisture)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle fake soil moisture: %.2fC", fakeSoilMoisture);

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
  s_instance->Log().Trace("Open day minimum: %d", openDayMinimum);

  OpenDayMinimum(openDayMinimum);
}

void HandleIndoorHumidityWarning(float indoorHumidityWarning)
{
  FlashLed(k_ledRecieve);
  s_instance->Log().Trace("Indoor humidity warning: %d", indoorHumidityWarning);

  IndoorHumidityWarning(indoorHumidityWarning);
}

void HandleSoilMostureWarning(float soilMostureWarning)
{
  FlashLed(k_ledRecieve);
  s_instance->Log().Trace("Soil moisture warning: %d", soilMostureWarning);

  SoilMostureWarning(soilMostureWarning);
}

BLYNK_CONNECTED()
{
  // read all last known values from Blynk server
  Blynk.syncVirtual(V0, V3, V5, V8, V9, V14, V15, V16, V17, V18);
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
  // TODO: https://github.com/nbolton/home-automation/issues/20
  s_instance->TraceFlash(F("Blynk write V5"));
  s_instance->HandleOpenStart((float)param.asInt());
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
  // TODO: https://github.com/nbolton/home-automation/issues/20
  s_instance->TraceFlash(F("Blynk write V8"));
  s_instance->HandleOpenFinish((float)param.asInt());
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
  s_instance->HandleIndoorHumidityWarning(param.asFloat());
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

  // TODO: find a better way to always call this last
  s_instance->HandleLastWrite();
}
