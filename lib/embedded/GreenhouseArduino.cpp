#include "GreenhouseArduino.h"

#include "BlynkCommon.h"

#include <Arduino.h>
#include <DHT.h>

#define DHT_PIN 14 // 14 = D5 on 8266
#define DHT_TYPE DHT11

const int enA = 4; // 4 = D2 on 8266
const int in1 = 0; // 0 = D3 on 8266
const int in2 = 12; // 12 = D6 on 8266
const int motorSpeed = 255;
const int checkFreqSec = 1;
const int openTimeSec = 12;
const int timerIntervalSec = 5;
const char auth[] = "IgXeM1Ri3cJZdHfs9ugS7gBfXXwzHqBS";
const char ssid[] = "Manhattan";
const char pass[] = "301 Park Ave";
const int unknown = -1;

int timerId = unknown;
int led = LOW;

DHT dht(DHT_PIN, DHT_TYPE);
BlynkTimer timer;

void flashLed(int times);
bool refresh();
void refreshTimer();
void reboot();

GreenhouseArduino* s_instance = nullptr;

GreenhouseArduino& Instance()
{
  return s_instance;
}

void Instance(GreenhouseArduino& ga)
{
  s_instance = ga;
}

void GreenhouseArduino::Setup()
{
  Serial.begin(9600);

  delay(1000);
  Serial.println("\n\nGreenhouse system");
  
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  
  analogWrite(enA, motorSpeed);

  Blynk.begin(auth, ssid, pass);

  dht.begin();

  // keep trying to refresh until it works
  Serial.println("Initial refresh"); 
  while (!refresh()) {
    
    Serial.println("Retry refresh");
    delay(1000);
  }
}

void GreenhouseArduino::Loop()
{
  Blynk.run();
  timer.run();
}

float GreenhouseArduino::GetTemperature() const
{
  return dht.readTemperature();
}

float GreenhouseArduino::GetHumidity() const
{
  return dht.readHumidity();
}

bool GreenhouseArduino::IsDhtReady() const
{
  return !isnan(t) && !isnan(h);
}

void GreenhouseArduino::ReportTemperature(float t)
{
  Blynk.virtualWrite(V1, t);
}

void GreenhouseArduino::ReportHumidity(float f)
{
  Blynk.virtualWrite(V2, h);
}

void GreenhouseArduino::FlashLed(int times)
{
  for (int i = 0; i < times * 2; i++) {
    led = (led == LOW) ? HIGH : LOW;
    digitalWrite(LED_BUILTIN, led);
    delay(100);
  }
}

void GreenhouseArduino::CloseWindow()
{
  Serial.println(F("Closing window..."));
  Blynk.virtualWrite(V4, 0);
  
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  
  delay(openTimeSec * 1000);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

  Serial.println(F("Window closed"));
  ws = windowClosed;
}

void GreenhouseArduino::OpenWindow()
{
  Serial.println(F("Opening window..."));
  Blynk.virtualWrite(V4, 1);
  
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);

  delay(openTimeSec * 1000);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

  Serial.println(F("Window opened"));
  ws = windowOpen;
}

BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V0, V3, V4, V5);
}

BLYNK_WRITE(V0)
{
  GreenhouseArduino ga = GreenhouseArduino::Instance();

  Serial.println("Blynk write V0");
  flashLed(2);
  
  ga.m_autoMode = (param.asInt() == 1);
  
  Serial.print("Auto mode: ");
  Serial.println(ga.m_autoMode ? "Auto" : "Manual");

  // light on for manual mode
  led = !ga.m_autoMode ? LOW : HIGH;
  digitalWrite(LED_BUILTIN, led);

  refresh();
}

BLYNK_WRITE(V3)
{
  Serial.println("Blynk write V3");
  int refreshRate = param.asInt();
  
  if (refreshRate <= 0) {
    Serial.print("Invalid refresh rate: ");
    Serial.print(refreshRate);
    Serial.println("s");
    return;
  }
  
  Serial.print("Setting refresh rate: ");
  Serial.print(refreshRate);
  Serial.println("s");
  
  if (timerId != -1) {
    Serial.print("Deleting old timer: ");
    Serial.print(timerId);
    Serial.println();
    timer.deleteTimer(timerId);
  }
  
  timerId = timer.setInterval(refreshRate * 1000L, refreshTimer);

  flashLed(3);
}

BLYNK_WRITE(V4)
{
  GreenhouseArduino ga = GreenhouseArduino::Instance();

  Serial.println("Blynk write V4");
  flashLed(4);
  
  int windowOpen = param.asInt();
  if (windowOpen == 1) {
    ga.OpenWindow();
  }
  else {
    ga.CloseWindow();
  }
}

BLYNK_WRITE(V5)
{
  Serial.println("Blynk write V5");
  flashLed(2);
  
  openTemp = param.asInt();
  Serial.printf("Open temperature: %dC\n", openTemp);

  refresh();
}

BLYNK_WRITE(V6)
{
  Serial.println("Blynk write V6");
  flashLed(2);
  
  if (param.asInt() == 1) {
    reboot();
  }
}

BLYNK_WRITE(V7)
{
  Serial.println("Blynk write V7");
  flashLed(2);
  
  if (param.asInt() == 1) {
    refresh();
  }
}

bool refresh()
{
  return GreenhouseArduino::Instance().Refresh();
}

void reboot()
{
  Blynk.disconnect();
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

void refreshTimer()
{
  Serial.println(F("Refresh timer"));
  refresh();
}
