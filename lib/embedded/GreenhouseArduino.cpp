#include "GreenhouseArduino.h"

#include <cstdarg>

#include "ho_config.h"

#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// 8266 free pins: D1, D4 (led), D7
// https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
const int k_actuatorPinEnA = 4;  // GPIO4  = D2
const int k_actuatorPinIn1 = 0;  // GPIO0  = D3 (connected to flash)
const int k_actuatorPinIn2 = 12; // GPIO12 = D6
const int k_ambientDhtPin = 14;  // GPIO14 = D5

const int k_actuatorSpeed = 255;
const int k_actuatorRuntimeSec = 12;
const int k_ledFlashDelay = 30; // ms

static char reportBuffer[80];

DHT ambientDht(k_ambientDhtPin, DHT11);
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
  m_temperature(k_unknown),
  m_humidity(k_unknown),
  m_timerId(k_unknown),
  m_led(LOW),
  m_fakeTemperature(k_unknown),
  m_refreshBusy(false)
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
  ambientDht.begin();

  // give DHT time to work
  // TODO: test if this is really actually needed
  delay(1000);

  Log().TraceFlash(F("System started"));
  ReportInfo("System started");
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
  while (m_refreshBusy) {
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
  for (int i = 0; i < (int)times * 2; i++) {
    m_led = (m_led == LOW) ? HIGH : LOW;
    digitalWrite(LED_BUILTIN, m_led);
    delay(k_ledFlashDelay);
  }
}

float GreenhouseArduino::Temperature() const
{
  if (TestMode()) {
    return m_fakeTemperature;
  }
  return m_temperature;
}

float GreenhouseArduino::Humidity() const { return m_humidity; }

bool GreenhouseArduino::ReadDhtSensor()
{
  if (TestMode()) {
    return true;
  }

  float t = ambientDht.readTemperature();
  float h = ambientDht.readHumidity();

  if (isnan(t) || isnan(h)) {
    t = k_unknown;
    h = k_unknown;
    return false;
  }

  m_temperature = t;
  m_humidity = h;

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

void GreenhouseArduino::ReportDhtValues()
{
  FlashLed(k_ledSend);
  Blynk.virtualWrite(V1, Temperature());
  Blynk.virtualWrite(V2, Humidity());
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

void GreenhouseArduino::HandleFakeTemperature(float fakeTemperature)
{
  FlashLed(k_ledRecieve);
  Log().Trace("Handle fake temperature: %.2fC", fakeTemperature);

  m_fakeTemperature = fakeTemperature;
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

BLYNK_CONNECTED()
{
  // read all last known values from Blynk server
  Blynk.syncVirtual(V0, V3, V5, V8, V9, V11, V14, V15);
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
