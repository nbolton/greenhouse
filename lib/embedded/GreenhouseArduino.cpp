#include "GreenhouseArduino.h"

#include <cstdarg>

#include "ho_config.h"

#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define DHT_PIN 14 // 14 = D5 on 8266
#define DHT_TYPE DHT11

const int unknown = -1;
const int enA = 4;  // 4 = D2 on 8266
const int in1 = 0;  // 0 = D3 on 8266
const int in2 = 12; // 12 = D6 on 8266
const int motorSpeed = 255;
const int checkFreqSec = 1;
const int openTimeSec = 12;
const int timerIntervalSec = 5;

static char reportBuffer[80];

DHT dht(DHT_PIN, DHT_TYPE);
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
  m_temperature(unknown),
  m_humidity(unknown),
  m_timerId(unknown),
  m_led(LOW),
  m_fakeTemperature(unknown)
{
}

void GreenhouseArduino::Setup()
{
  Serial.begin(9600);

  delay(1000);
  Log().Trace("\n\n%s: Starting system", BLYNK_DEVICE_NAME);

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  analogWrite(enA, motorSpeed);

  Blynk.begin(auth, ssid, pass);

  dht.begin();

  Log().TraceFlash(F("System started"));
  ReportInfo("System started");

  // give DHT time to work
  // TODO: test if this is really actually needed
  delay(1000);
}

void GreenhouseArduino::Loop()
{
  Blynk.run();
  timer.run();
}

bool GreenhouseArduino::Refresh()
{
  Log().Trace("Free heap: %d bytes", ESP.getFreeHeap());
  return Greenhouse::Refresh();
}

void GreenhouseArduino::ReportInfo(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(reportBuffer, format, args);
  va_end(args);

  Blynk.logEvent("log_info", reportBuffer);
}

void GreenhouseArduino::ReportWarning(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(reportBuffer, format, args);
  va_end(args);

  Blynk.logEvent("log_warning", reportBuffer);
}

void GreenhouseArduino::ReportCritical(const char *format, ...) const
{
  va_list args;
  va_start(args, format);
  vsprintf(reportBuffer, format, args);
  va_end(args);

  Blynk.logEvent("log_critical", reportBuffer);
}

float GreenhouseArduino::Temperature() const
{
  if (dhtFakeMode) {
    return m_fakeTemperature;
  }
  return m_temperature;
}

float GreenhouseArduino::Humidity() const { return m_humidity; }

bool GreenhouseArduino::ReadDhtSensor()
{
  if (dhtFakeMode) {
    return true;
  }

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    t = unknown;
    h = unknown;
    return false;
  }

  m_temperature = t;
  m_humidity = h;

  return true;
}

void GreenhouseArduino::ReportDhtValues()
{
  Blynk.virtualWrite(V1, Temperature());
  Blynk.virtualWrite(V2, Humidity());
}

void GreenhouseArduino::FlashLed(int times)
{
  for (int i = 0; i < times * 2; i++) {
    m_led = (m_led == LOW) ? HIGH : LOW;
    digitalWrite(LED_BUILTIN, m_led);
    delay(100);
  }
}

void GreenhouseArduino::CloseWindow(float delta)
{
  TraceFlash(F("Closing window..."));
  Log().Trace("Delta: %.2f", delta);

  Greenhouse::CloseWindow(delta);

  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);

  int runtime = (openTimeSec * 1000) * delta;
  Log().Trace("Close runtime: %dms", runtime);
  delay(runtime);

  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

  float percent = delta * 100;
  Log().Trace("Window closed %.1f%%", percent);
}

void GreenhouseArduino::OpenWindow(float delta)
{
  TraceFlash(F("Opening window..."));
  Log().Trace("Delta: %.2f", delta);

  Greenhouse::OpenWindow(delta);

  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);

  int runtime = (openTimeSec * 1000) * delta;
  Log().Trace("Open runtime: %dms", runtime);
  delay(runtime);

  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

  float percent = delta * 100;
  Log().Trace("Window opened %.1f%%", percent);
}

void GreenhouseArduino::ReportWindowProgress() { Blynk.virtualWrite(V9, WindowProgress()); }

void GreenhouseArduino::ReportLastRefresh()
{
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  // current date
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
}

void GreenhouseArduino::HandleAutoMode(bool autoMode)
{
  FlashLed(2);
  AutoMode(autoMode == 1);

  Log().Trace("Auto mode: %s", (autoMode ? "Auto" : "Manual"));

  // light on for manual mode
  m_led = !autoMode ? LOW : HIGH;
  digitalWrite(LED_BUILTIN, m_led);
}

void GreenhouseArduino::HandleOpenStart(float openStart)
{
  FlashLed(2);

  OpenStart(openStart);
  Log().Trace("Open starting temperature: %.2fC", openStart);
}

void GreenhouseArduino::HandleOpenFinish(float openFinish)
{
  FlashLed(2);

  OpenFinish(openFinish);
  Log().Trace("Open finishing temperature: %.2fC", openFinish);
}

void GreenhouseArduino::HandleRefreshRate(int refreshRate)
{
  if (refreshRate <= 0) {
    Log().Trace("Invalid refresh rate: %ds", refreshRate);
    return;
  }

  Log().Trace("Setting refresh rate: %ds", refreshRate);

  if (m_timerId != -1) {
    Log().Trace("Deleting old timer: %d", m_timerId);
    timer.deleteTimer(m_timerId);
  }

  m_timerId = timer.setInterval(refreshRate * 1000L, refreshTimer);

  FlashLed(3);
}

void GreenhouseArduino::HandleWindowProgress(int windowProgress)
{
  FlashLed(4);
  ApplyWindowProgress((float)windowProgress / 100);
  WindowProgress(windowProgress);
}

void GreenhouseArduino::HandleReset(int reset)
{
  FlashLed(2);
  if (reset == 1) {
    Reset();
  }
}

void GreenhouseArduino::HandleRefresh(int refresh)
{
  FlashLed(2);

  if (refresh == 1) {
    Refresh();
  }
}

void GreenhouseArduino::HandleFakeTemperature(float fakeTemperature)
{
  m_fakeTemperature = fakeTemperature;
}

void GreenhouseArduino::Reset()
{
  ReportWarning("System rebooting");
  Blynk.disconnect();
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {
  }
}

BLYNK_CONNECTED()
{
  // read all last known values from Blynk server
  Blynk.syncVirtual(V0, V3, V5, V8, V9, V11);
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

BLYNK_WRITE(V11)
{
  s_instance->TraceFlash(F("Blynk write V11"));
  s_instance->HandleFakeTemperature(param.asFloat());
}
