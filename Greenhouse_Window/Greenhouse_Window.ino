#include <DHT.h>

#define DHT_PIN 2
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

const int enA = 9;
const int in1 = 8;
const int in2 = 7;

const float openTemp = 27.00;
const float openHumid = 70;
const int motorSpeed = 255;
const int checkFreqSec = 1;
const int openTimeSec = 12;

enum windowState {
  windowUnknown, windowOpen, windowClosed
};

windowState ws = windowUnknown;

void setup() {

  Serial.begin(9600);

  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);

  dht.begin();
}

void closeWindow() {
  Serial.println(F("Closing window..."));
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  
  delay(openTimeSec * 1000);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

  Serial.println(F("Window closed"));
  ws = windowClosed;
}

void openWindow() {
  Serial.println(F("Opening window..."));
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);

  delay(openTimeSec * 1000);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);

  Serial.println(F("Window opened"));
  ws = windowOpen;
}

void loop() {

  delay(checkFreqSec * 1000);
  digitalWrite(LED_BUILTIN, HIGH);
  
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  
  if (isnan(t) || isnan(h)) {
    Serial.println(F("Failed to read DHT"));
    return;
  }
  
  Serial.print(F("Temperature: "));
  Serial.println(t);
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.println("%");
  
  analogWrite(enA, motorSpeed);

  if (ws == windowUnknown) {
    // open is probably the safest state
    openWindow();
  }

  if ((t > openTemp) || (h > openHumid)) {
    if (ws == windowClosed) {
      openWindow();
    }
  }
  else {
    if (ws == windowOpen) {
      closeWindow();
    }
  }
  
  digitalWrite(LED_BUILTIN, LOW);
}
