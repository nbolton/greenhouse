#include "GreenhouseArduino.h"

#include <cstdarg>

#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLovPyrAUR"
#define BLYNK_DEVICE_NAME "Greenhouse"

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
const char auth[] = "IgXeM1Ri3cJZdHfs9ugS7gBfXXwzHqBS";
const char ssid[] = "Manhattan";
const char pass[] = "301 Park Ave";

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
  m_log(), m_temperature(unknown), m_humidity(unknown), m_timerId(unknown), m_led(LOW)
{
}

void GreenhouseArduino::Setup()
{
  Serial.begin(9600);

  delay(1000);
  TraceFlash(F("\n\nGreenhouse system"));

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  analogWrite(enA, motorSpeed);

  Blynk.begin(auth, ssid, pass);

  dht.begin();

  // keep trying to refresh until it works
  TraceFlash(F("Initial refresh"));
  while (!Refresh()) {

    TraceFlash(F("Retry refresh"));
    delay(1000);
  }
}

void GreenhouseArduino::Loop()
{
  Blynk.run();
  timer.run();
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

float GreenhouseArduino::Temperature() const { return m_temperature; }

float GreenhouseArduino::Humidity() const { return m_humidity; }

bool GreenhouseArduino::ReadDhtSensor()
{
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
  Blynk.virtualWrite(V1, m_temperature);
  Blynk.virtualWrite(V2, m_humidity);
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
  Log().Trace("Delta: %f", delta);

  Greenhouse::CloseWindow(delta);
  ReportWindowProgress();

  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);

  delay((openTimeSec * 1000) * delta);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

  TraceFlash(F("Window closed"));
  ReportInfo("Window closed, delta: %f", delta);
}

void GreenhouseArduino::OpenWindow(float delta)
{
  TraceFlash(F("Opening window..."));
  Log().Trace("Delta: %f", delta);

  Greenhouse::OpenWindow(delta);
  ReportWindowProgress();

  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);

  delay((openTimeSec * 1000) * delta);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

  TraceFlash(F("Window opened"));
  ReportInfo("Window opened, delta: %f", delta);
}

void GreenhouseArduino::ReportWindowProgress() { Blynk.virtualWrite(V9, WindowProgress()); }

void GreenhouseArduino::ReportLastRefresh()
{
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  Blynk.virtualWrite(V10, timeClient.getFormattedTime());
}

void GreenhouseArduino::HandleAutoMode(bool autoMode)
{
  FlashLed(2);
  AutoMode(autoMode == 1);

  Log().Trace("Auto mode: %s", (autoMode ? "Auto" : "Manual"));

  // light on for manual mode
  m_led = !autoMode ? LOW : HIGH;
  digitalWrite(LED_BUILTIN, m_led);

  Refresh();
}

void GreenhouseArduino::HandleOpenStart(float openStart)
{
  FlashLed(2);

  OpenStart(openStart);
  Log().Trace("Open starting temperature: %dC", openStart);

  Refresh();
}

void GreenhouseArduino::HandleOpenFinish(float openFinish)
{
  FlashLed(2);

  OpenFinish(openFinish);
  Log().Trace("Open finishing temperature: %dC", openFinish);

  Refresh();
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
    Reboot();
  }
}

void GreenhouseArduino::HandleRefresh(int refresh)
{
  FlashLed(2);

  if (refresh == 1) {
    Refresh();
  }
}

void GreenhouseArduino::Reboot()
{
  Blynk.disconnect();
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {
  }
}

BLYNK_CONNECTED() { Blynk.syncVirtual(V0, V3, V4, V5); }

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
