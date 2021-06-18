#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLovPyrAUR"
#define BLYNK_DEVICE_NAME "Greenhouse"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
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

enum windowState {
  windowUnknown, windowOpen, windowClosed
};

windowState ws = windowUnknown;
int timerId = unknown;
int led = LOW;
bool autoMode = false;
int openTemp = unknown;
bool dhtFailSent = false;

DHT dht(DHT_PIN, DHT_TYPE);
BlynkTimer timer;

void flashLed(int times);
bool refresh();
void refreshTimer();
void closeWindow();
void openWindow();
void reboot();

void setup()
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

void loop()
{
  Blynk.run();
  timer.run();
}

BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V0, V3, V4, V5);
}

BLYNK_WRITE(V0)
{
  Serial.println("Blynk write V0");
  flashLed(2);
  
  autoMode = (param.asInt() == 1);
  
  Serial.print("Auto mode: ");
  Serial.println(autoMode ? "Auto" : "Manual");

  // light on for manual mode
  led = !autoMode ? LOW : HIGH;
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
  Serial.println("Blynk write V4");
  flashLed(4);
  
  int windowOpen = param.asInt();
  if (windowOpen == 1) {
    openWindow();
  }
  else {
    closeWindow();
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

void flashLed(int times)
{
  for (int i = 0; i < times * 2; i++) {
    led = (led == LOW) ? HIGH : LOW;
    digitalWrite(LED_BUILTIN, led);
    delay(100);
  }
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
  Serial.println("Refresh");
  bool ok = true;
  
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  
  if (isnan(t) || isnan(h)) {
    
    ok = false;
    const char dhtFail[] = "DHT device unavailable";
    Serial.println(dhtFail);

    // only send once per reboot (don't spam the timeline)
    if (!dhtFailSent) {
      Blynk.logEvent("log_warning", dhtFail);
      dhtFailSent = true;
    }
    
    t = unknown;
    h = unknown;
  }
  
  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.print(F(" | Humidity: "));
  Serial.print(h);
  Serial.println("%");

  Blynk.virtualWrite(V1, t);
  Blynk.virtualWrite(V2, h);

  flashLed(2);

  if (t != unknown) {
    Serial.println("t known");
    if (autoMode && (openTemp != unknown)) {
      Serial.println("auto mode & open temp known");
      if (t > openTemp) {
        Serial.println("t > openTemp");
        if (ws == windowClosed) {
          Serial.println("ws == windowClosed");
          openWindow();
        }
        else {
          Serial.println("window already open");
        }
      }
      else {
        Serial.println("t <= openTemp");
        if (ws == windowOpen) {
          Serial.println("ws == windowOpen");
          closeWindow();
        }
        else {
          Serial.println("window already closed");
        }
      }
    }
    else {
      Serial.println("manual mode or open temp unknown");
    }
  }
  else {
      Serial.println("t unknown");
  }

  return ok;
}

void closeWindow()
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

void openWindow()
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
