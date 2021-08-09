#include "GreenhouseArduino.h"

#include <cstdarg>

#include "ho_config.h"

#include <ADS1115_WE.h>
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

const int debug = true;

// regular pins
const int k_oneWirePin = D3;
const int k_msrLatchPin = D5; // RCLK (12)
const int k_msrDataPin = D6;  // SER (14)
const int k_msrClockPin = D7; // SRCLK (11)
const int k_actuatorPinA = D8;

// msr pins
const int relayPin = 0;
const int startBeepPin = 1;
const int switchPins[] = {0 + 8, 1 + 8, 2 + 8, 3 + 8};

// io1 pins
const int aMuxEn = P0;
const int aMuxS0 = P1;
const int aMuxS1 = P2;
const int aMuxS2 = P3;
const int aMuxS3 = P4;
const int k_actuatorPin1 = P5;
const int k_actuatorPin2 = P6;

// amux pins
const int k_moisturePin = 3;

const int msrTotal = 2;
const int voltAverageCountMax = 100;
const int pvVoltageMin = 5;            // V, in case of sudden voltage drop
const float voltSwitchOff = 10.5; // V
const float voltSwitchOn = 13.5;  // V

const int switchButtons = 4;
const int fanSwitch = 0;
const int pumpSwitch1 = 2;
const int pumpSwitch2 = 3;

int switchState[switchButtons];
int activeSwitch;
int waterBatteryOn = k_unknown;
int waterBatteryOff = k_unknown;
int forceRelay = false;

bool pvPowerSource;
float pvVoltageSwitchOn = k_unknown;
float pvVoltageSwitchOff = k_unknown;
int pvVoltageCount = 0;
float pvVoltageAverage = 0;
float pvVoltageSum = 0;
float pvVoltageSensor = k_unknown;
float pvVoltageOutput = k_unknown;
float pvVoltageSensorMin = 0;
float pvVoltageSensorMax = k_unknown;
float pvVoltageOutputMin = 0;
float pvVoltageOutputMax = k_unknown;
float pvCurrentSensor = k_unknown;
float pvCurrentOutput = k_unknown;
float pvCurrentSensorMin = 0;
float pvCurrentSensorMax = k_unknown;
float pvCurrentOutputMin = 0;
float pvCurrentOutputMax = k_unknown;

PCF8574 io1(0x20);
MultiShiftRegister msr(msrTotal, k_msrLatchPin, k_msrClockPin, k_msrDataPin);

Thread relayThread = Thread();
Thread readThread = Thread();
Thread testThread = Thread();

OneWire oneWire(k_oneWirePin);
DallasTemperature sensors(&oneWire);

WiFiUDP ntpUdp;
NTPClient timeClient(ntpUdp);

const int k_actuatorSpeed = 255;
const int k_actuatorRuntimeSec = 12;
const int k_ledFlashDelay = 30; // ms
const int k_soilProbeIndex = 0;

static char reportBuffer[80];

BlynkTimer timer;

bool enableHeater = false;
uint8_t loopCnt = 0;

Adafruit_SHT31 sht31_a = Adafruit_SHT31();
Adafruit_SHT31 sht31_b = Adafruit_SHT31();

ADS1115_WE adc1 = ADS1115_WE(0x48);

GreenhouseArduino *s_instance = nullptr;

void startBeep(int times);
void readCallback();
void actuator(bool forward, int s, int t);
void printTemps();
void setSwitch(int i, bool on);
void toggleSwitch(int i);
void relayCallback();
int analogMuxRead(byte chan);
void switchPower(bool pv);
void measureVoltage();
void measureCurrent();
void beginSensor(Adafruit_SHT31 &sht31, uint8_t addr, String shtName);
void printValues(Adafruit_SHT31 &sht31, String shtName);
void testCallback();

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

  Serial.begin(9600);

  // wait for serial to connect before first trace
  delay(1000);
  Log().Trace("\n\nStarting system: %s", BLYNK_DEVICE_NAME);

  pinMode(k_msrLatchPin, OUTPUT);
  pinMode(k_msrClockPin, OUTPUT);
  pinMode(k_msrDataPin, OUTPUT);
  msr.shift();

  startBeep(1);

  Wire.begin();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(k_actuatorPinA, OUTPUT);

  io1.pinMode(aMuxS0, OUTPUT);
  io1.pinMode(aMuxS1, OUTPUT);
  io1.pinMode(aMuxS2, OUTPUT);
  io1.pinMode(aMuxS3, OUTPUT);
  io1.pinMode(aMuxEn, OUTPUT);
  io1.pinMode(k_actuatorPin1, OUTPUT);
  io1.pinMode(k_actuatorPin2, OUTPUT);

  io1.digitalWrite(aMuxS0, LOW);
  io1.digitalWrite(aMuxS1, LOW);
  io1.digitalWrite(aMuxS2, LOW);
  io1.digitalWrite(aMuxS3, LOW);
  io1.digitalWrite(aMuxEn, LOW);
  io1.digitalWrite(k_actuatorPin1, LOW);
  io1.digitalWrite(k_actuatorPin2, LOW);

  // 10ms seems to be the minimum switch time to stop the relay coil "buzzing"
  // when it has insufficient power.
  relayThread.onRun(relayCallback);
  relayThread.setInterval(10);

  readThread.onRun(readCallback);
  readThread.setInterval(100);

  testThread.onRun(testCallback);
  testThread.setInterval(1000);

  sensors.begin();

  timeClient.begin();

  measureVoltage();

  beginSensor(sht31_a, 0x44, "SHT31 A");
  beginSensor(sht31_b, 0x45, "SHT31 B");

  if (!adc1.init()) {
    Serial.println("ADS1115 not connected");
  }

  // TODO: not default, experiment without this
  adc1.setVoltageRange_mV(ADS1115_RANGE_6144);

  Blynk.begin(k_auth, k_ssid, k_pass);

  Serial.println("System ready");
  startBeep(2);
}

void GreenhouseArduino::Loop()
{
  Greenhouse::Loop();

  timeClient.update();
  Blynk.run();
  timer.run();

  if (timeClient.getHours() == waterBatteryOn) {
    setSwitch(fanSwitch, true);
    delay(5000); // HACK: wait for fan to spool up
    setSwitch(pumpSwitch1, true);
    setSwitch(pumpSwitch2, true);
  }
  else if (timeClient.getHours() == waterBatteryOff) {
    setSwitch(fanSwitch, false);
    setSwitch(pumpSwitch1, false);
    setSwitch(pumpSwitch2, false);
  }

  if (relayThread.shouldRun()) {
    relayThread.run();
  }

  if (readThread.shouldRun()) {
    readThread.run();
  }

  if (testThread.shouldRun()) {
    testThread.run();
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

  measureCurrent();

  if (debug) {
    Serial.print("PV voltage sensor: ");
    Serial.print(pvVoltageSensor);
    Serial.println("V");

    Serial.print("PV voltage output: ");
    Serial.print(pvVoltageOutput);
    Serial.println("V");

    Serial.print("PV voltage average: ");
    Serial.print(pvVoltageAverage);
    Serial.println("V");

    Serial.print("PV current sensor: ");
    Serial.print(pvCurrentSensor);
    Serial.println("V");

    Serial.print("PV current output: ");
    Serial.print(pvCurrentOutput);
    Serial.println("A");
  }

  Blynk.virtualWrite(V28, pvPowerSource);
  Blynk.virtualWrite(V29, pvVoltageSensor);
  Blynk.virtualWrite(V30, pvVoltageOutput);
  Blynk.virtualWrite(V42, pvCurrentSensor);
  Blynk.virtualWrite(V43, pvCurrentOutput);

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

  m_insideTemperature = sht31_a.readTemperature();
  if (isnan(m_insideTemperature)) {
    m_insideTemperature = k_unknown;
    failures++;
  }

  m_insideHumidity = sht31_a.readHumidity();
  if (isnan(m_insideHumidity)) {
    m_insideHumidity = k_unknown;
    failures++;
  }

  m_outsideTemperature = sht31_b.readTemperature();
  if (isnan(m_outsideTemperature)) {
    m_outsideTemperature = k_unknown;
    failures++;
  }

  m_outsideHumidity = sht31_b.readHumidity();
  if (isnan(m_outsideHumidity)) {
    m_outsideHumidity = k_unknown;
    failures++;
  }

  sensors.requestTemperatures();
  m_soilTemperature = sensors.getTempCByIndex(k_soilProbeIndex);
  if (isnan(m_soilTemperature)) {
    m_soilTemperature = k_unknown;
    failures++;
  }

  int moistureAnalog = analogMuxRead(k_moisturePin);
  m_soilMoisture = CalculateMoisture(moistureAnalog);

  return failures == 0;
}

void GreenhouseArduino::SystemDigitalWrite(int pin, int val) { io1.digitalWrite(pin, val); }

void GreenhouseArduino::SystemDelay(unsigned long ms) { delay(ms); }

void GreenhouseArduino::CloseWindow(float delta)
{
  TraceFlash(F("Closing window..."));
  Log().Trace("Delta: %.2f", delta);

  Greenhouse::CloseWindow(delta);

  analogWrite(k_actuatorPinA, k_actuatorSpeed);

  SystemDigitalWrite(k_actuatorPin1, LOW);
  SystemDigitalWrite(k_actuatorPin2, HIGH);

  int runtime = (k_actuatorRuntimeSec * 1000) * delta;
  Log().Trace("Close runtime: %dms", runtime);
  SystemDelay(runtime);

  SystemDigitalWrite(k_actuatorPin1, LOW);
  SystemDigitalWrite(k_actuatorPin2, LOW);

  float percent = delta * 100;
  Log().Trace("Window closed %.1f%%", percent);
}

void GreenhouseArduino::OpenWindow(float delta)
{
  TraceFlash(F("Opening window..."));
  Log().Trace("Delta: %.2f", delta);

  Greenhouse::OpenWindow(delta);

  analogWrite(k_actuatorPinA, k_actuatorSpeed);

  SystemDigitalWrite(k_actuatorPin1, HIGH);
  SystemDigitalWrite(k_actuatorPin2, LOW);

  int runtime = (k_actuatorRuntimeSec * 1000) * delta;
  Log().Trace("Open runtime: %dms", runtime);
  SystemDelay(runtime);

  SystemDigitalWrite(k_actuatorPin1, LOW);
  SystemDigitalWrite(k_actuatorPin2, LOW);

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

void startBeep(int times)
{

  for (int i = 0; i < times; i++) {
    msr.set_shift(startBeepPin);
    delay(100);

    msr.clear_shift(startBeepPin);
    delay(200);
  }
}

void readCallback()
{

  if (Serial.available() > 0) {

    String s = Serial.readString();

    switch (s.charAt(0)) {
    case 'b':
      startBeep(1);
      return;
    case 't':
      printTemps();
      return;

    // e.g. af5
    case 'a': {
      int sp = (int)s.charAt(2) - 48;
      int t = s.charAt(3);

      switch (s.charAt(1)) {
      case 'f':
        actuator(true, sp, t);
        return;
      case 'r':
        actuator(false, sp, t);
        return;
      }
    }

    // e.g. s1
    case 's': {
      switch (s.charAt(1)) {
      case '0':
        toggleSwitch(0);
        return;
      case '1':
        toggleSwitch(1);
        return;
      case '2':
        toggleSwitch(2);
        return;
      case '3':
        toggleSwitch(3);
        return;
      }
    }

    case 'r': {
      forceRelay = !forceRelay;
    }
    }

    Serial.print("Unknown: ");
    Serial.println(s);
  }
}

void actuator(bool forward, int s, int t)
{

  s *= 28;   // 9 = 252 (out of 255)
  t *= 1000; // 1 = 1000ms

  Serial.print("Actuator D=");
  Serial.print(forward ? "f" : "r");
  Serial.print(" S=");
  Serial.print(s);
  Serial.print(" T=");
  Serial.println(t);

  analogWrite(k_actuatorPinA, s);

  if (forward) {
    io1.digitalWrite(k_actuatorPin1, HIGH);
    io1.digitalWrite(k_actuatorPin2, LOW);
  }
  else {
    io1.digitalWrite(k_actuatorPin1, LOW);
    io1.digitalWrite(k_actuatorPin2, HIGH);
  }

  delay(t);

  // stop
  io1.digitalWrite(k_actuatorPin1, LOW);
  io1.digitalWrite(k_actuatorPin2, LOW);

  Serial.println("Actuator done");
}

void printTemps()
{

  sensors.requestTemperatures();

  Serial.print("T0: ");
  Serial.print(sensors.getTempCByIndex(0));
  Serial.println("°C");

  Serial.print("T1: ");
  Serial.print(sensors.getTempCByIndex(1));
  Serial.println("°C");
}

void setSwitch(int index, bool on)
{

  int pin = switchPins[index];

  if (on) {
    Serial.print("SR pin set: ");
    Serial.println(pin);

    msr.set_shift(pin);
    switchState[index] = true;
  }
  else {
    Serial.print("SR pin clear: ");
    Serial.println(pin);

    msr.clear_shift(pin);
    switchState[index] = false;
  }

  String switchStates;
  for (int i = 0; i < switchButtons; i++) {
    switchStates += "S";
    switchStates += i;
    switchStates += "=";
    switchStates += switchState[i] ? "On" : "Off";
    if (i != (switchButtons - 1)) {
      switchStates += ", ";
    }
  }
  Blynk.virtualWrite(V33, switchStates);
}

void toggleSwitch(int i)
{

  if (!switchState[i]) {
    Serial.print("Toggle switch on: ");
    Serial.println(i);
    setSwitch(i, true);
  }
  else {
    Serial.print("Toggle switch off: ");
    Serial.println(i);
    setSwitch(i, false);
  }
}

void relayCallback()
{
  measureVoltage();

  if (forceRelay) {
    if (!pvPowerSource) {
      switchPower(true);
    }
    return;
  }

  if (pvPowerSource && (pvVoltageOutput <= pvVoltageMin)) {
    Serial.println("PV voltage drop detected");
    switchPower(false);
    pvVoltageAverage = k_unknown;
  }
  else if (pvVoltageAverage != k_unknown) {

    if (!pvPowerSource && (pvVoltageSwitchOn != k_unknown) && (pvVoltageAverage >= pvVoltageSwitchOn)) {
      switchPower(true);
    }
    else if (pvPowerSource && (pvVoltageSwitchOff != k_unknown) && (pvVoltageAverage <= pvVoltageSwitchOff)) {
      switchPower(false);
    }
  }
}

int analogMuxRead(byte chan)
{
  io1.digitalWrite(aMuxS0, bitRead(chan, 0));
  io1.digitalWrite(aMuxS1, bitRead(chan, 1));
  io1.digitalWrite(aMuxS2, bitRead(chan, 2));
  io1.digitalWrite(aMuxS3, bitRead(chan, 3));

  return analogRead(A0);
}

float readAdc1(ADS1115_MUX channel)
{
  float voltage = 0.0;
  adc1.setCompareChannels(channel);
  adc1.startSingleMeasurement();

  while (adc1.isBusy()) {
  }

  voltage = adc1.getResult_V(); // alternative: getResult_mV for Millivolt
  return voltage;
}

void measureVoltage()
{
  if (pvVoltageSensorMax == k_unknown || pvVoltageOutputMax == k_unknown) {
    return;
  }

  pvVoltageSensor = readAdc1(ADS1115_COMP_0_GND);
  pvVoltageOutput = mapFloat(
    pvVoltageSensor,
    pvVoltageSensorMin,
    pvVoltageSensorMax,
    pvVoltageOutputMin,
    pvVoltageOutputMax);

  pvVoltageSum += pvVoltageOutput;
  pvVoltageCount++;

  if (pvVoltageCount >= voltAverageCountMax) {

    pvVoltageAverage = (pvVoltageSum / pvVoltageCount);

    pvVoltageCount = 0;
    pvVoltageSum = 0;
  }
}

void measureCurrent()
{
  if (pvCurrentSensorMax == k_unknown || pvCurrentOutputMax == k_unknown) {
    return;
  }

  pvCurrentSensor = readAdc1(ADS1115_COMP_1_GND);
  pvCurrentOutput = mapFloat(
    pvCurrentSensor,
    pvCurrentSensorMin,
    pvCurrentSensorMax,
    pvCurrentOutputMin,
    pvCurrentOutputMax);
}

void switchPower(bool pv)
{
  Serial.println("----");

  if (pv) {
    msr.set_shift(relayPin);
  }
  else {
    msr.clear_shift(relayPin);
  }

  Serial.println(pv ? "PV on" : "PV off");
  pvPowerSource = pv;

  Serial.print("Source: ");
  Serial.println(pv ? "PV" : "PSU");

  Blynk.virtualWrite(V28, pvPowerSource);

  Serial.println("----");
}

void beginSensor(Adafruit_SHT31 &sht31, uint8_t addr, String shtName)
{

  if (!sht31.begin(addr)) {
    Serial.println("Couldn't find " + shtName);
  }
  /*
  Serial.print(shtName + " heater is ");
  if (sht31.isHeaterEnabled())
    Serial.println("on");
  else
    Serial.println("off");*/
}

void printValues(Adafruit_SHT31 &sht31, String shtName)
{

  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  Serial.print(shtName);

  if (!isnan(t)) {
    Serial.print(": Temp ºC = ");
    Serial.print(t);
  }
  else {
    Serial.println("\nFailed to read temperature of " + shtName);
  }

  if (!isnan(h)) {
    Serial.print(" / Humidity % = ");
    Serial.println(h);
  }
  else {
    Serial.println("\nFailed to read humidity of " + shtName);
  }

  // Toggle heater enabled state every 30 seconds
  // An ~3.0 degC temperature increase can be noted when heater is enabled
  if (++loopCnt == 30) {
    enableHeater = !enableHeater;
    sht31.heater(enableHeater);
    if (sht31.isHeaterEnabled())
      Serial.println(shtName + " heater turned on");
    else
      Serial.println(shtName + " heater turned off");

    loopCnt = 0;
  }
}

void testCallback()
{
  //
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
  s_instance->Log().Trace("Handle open day minimum: %d", openDayMinimum);

  OpenDayMinimum(openDayMinimum);
}

void GreenhouseArduino::HandleInsideHumidityWarning(float insideHumidityWarning)
{
  FlashLed(k_ledRecieve);
  s_instance->Log().Trace("Handle inside humidity warning: %d", insideHumidityWarning);

  InsideHumidityWarning(insideHumidityWarning);
}

void GreenhouseArduino::HandleSoilMostureWarning(float soilMostureWarning)
{
  FlashLed(k_ledRecieve);
  s_instance->Log().Trace("Handle soil moisture warning: %d", soilMostureWarning);

  SoilMostureWarning(soilMostureWarning);
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
    V45);
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
  activeSwitch = param.asInt();
}

BLYNK_WRITE(V25)
{
  s_instance->TraceFlash(F("Blynk write V25"));
  if (param.asInt() == 1) {
    toggleSwitch(activeSwitch);
  }
}

BLYNK_WRITE(V31)
{
  s_instance->TraceFlash(F("Blynk write V31"));
  waterBatteryOn = param.asInt();
}

BLYNK_WRITE(V32)
{
  s_instance->TraceFlash(F("Blynk write V32"));
  waterBatteryOff = param.asInt();
}

BLYNK_WRITE(V34)
{
  s_instance->TraceFlash(F("Blynk write V34"));
  pvVoltageSensorMin = param.asFloat();
}

BLYNK_WRITE(V35)
{
  s_instance->TraceFlash(F("Blynk write V35"));
  pvVoltageSensorMax = param.asFloat();
}

BLYNK_WRITE(V36)
{
  s_instance->TraceFlash(F("Blynk write V36"));
  pvVoltageOutputMin = param.asFloat();
}

BLYNK_WRITE(V37)
{
  s_instance->TraceFlash(F("Blynk write V37"));
  pvVoltageOutputMax = param.asFloat();
}

BLYNK_WRITE(V38)
{
  s_instance->TraceFlash(F("Blynk write V38"));
  pvCurrentSensorMin = param.asFloat();
}

BLYNK_WRITE(V39)
{
  s_instance->TraceFlash(F("Blynk write V39"));
  pvCurrentSensorMax = param.asFloat();
}

BLYNK_WRITE(V40)
{
  s_instance->TraceFlash(F("Blynk write V40"));
  pvCurrentOutputMin = param.asFloat();
}

BLYNK_WRITE(V41)
{
  s_instance->TraceFlash(F("Blynk write V41"));
  pvCurrentOutputMax = param.asFloat();
}

BLYNK_WRITE(V44)
{
  s_instance->TraceFlash(F("Blynk write V44"));
  pvVoltageSwitchOn = param.asFloat();
}

BLYNK_WRITE(V45)
{
  s_instance->TraceFlash(F("Blynk write V45"));
  pvVoltageSwitchOff = param.asFloat();

  // TODO: find a better way to always call this last; sometimes
  // when adding new write functions, moving this gets forgotten about.
  s_instance->HandleLastWrite();
}
