// caution!
// dumbness in iotnx4.h (provided by platformio)
// .platformio\packages\toolchain-atmelavr\avr\include\avr\iotnx4.h
// the header defines P[AB][n] with all the wrong pin numbers, but also has an
// obscure #warning that tells you that the pins are counterclockwise (i.e.
// don't match the pins defined by the library.
// ATMEL ATTINY84 / ARDUINO
//
//                           +-\/-+
//                     VCC  1|    |14  GND
//             (D  0)  PB0  2|    |13  PA0  (D 10)        AREF
//             (D  1)  PB1  3|    |12  PA1  (D  9)
//             (D 11)  PB3  4|    |11  PA2  (D  8)
//  PWM  INT0  (D  2)  PB2  5|    |10  PA3  (D  7)
//  PWM        (D  3)  PA7  6|    |9   PA4  (D  6)
//  PWM        (D  4)  PA6  7|    |8   PA5  (D  5)        PWM
//                           +----+
#include <Arduino.h>
#define PA0 10
#define PA1 9
#define PA2 8
#define PA3 7
#define PA4 6
#define PA5 5
#define PA6 4
#define PA7 3
#define PB0 0
#define PB1 1
#define PB2 2
#define PB3 11

#define TX_TEST 0
#define LED_DEBUG 0
#define SR_DEBUG 0
#define FLASH_DELAY 200
#define RADIO_EN 1
#define TX_WAIT 1
#define TX_WAIT_DELAY 10

#if RADIO_EN
// caution! compile RH_ASK.cpp with RH_ASK_ATTINY_USE_TIMER1 uncommented.
// otherwise TIMER0 will be used, and delay()/millis() stop working.
#include <RH_ASK.h>
#endif // RADIO_EN

#include <OneWire.h>

#define SR_TOTAL 1
#define SR_PIN_LED_ON 15
#define SR_PIN_LED_ERR 1
#define SR_PIN_LED_RX 2
#define SR_PIN_LED_TX 3
#define SR_PIN_MOTOR_B 4
#define SR_PIN_MOTOR_A 5

#define PIN_SR_LATCH PA7  // RCLK (12)
#define PIN_SR_DATA PA6   // SER (14)
#define PIN_SR_CLOCK PA5  // SRCLK (11)
#define PIN_TX_EN PA0
#define PIN_TX PA1
#define PIN_RX PA2

#define PIN_DEBUG PA1

#define PIN_ONE_WIRE PA3
#define PIN_MOTOR_PWM PB2

#define TX_BIT_RATE 2000
#define RX_BUF_LEN 8
#define TX_BUF_LEN 14

#define TEMP_UNKNOWN 255

#if RADIO_EN

RH_ASK driver(TX_BIT_RATE, PIN_RX, PIN_TX, PIN_TX_EN);
uint8_t sequence = 0;
uint8_t rxBuf[RX_BUF_LEN];
uint8_t txBuf[TX_BUF_LEN];
char* rxBufChar = (char*)rxBuf;
char* txBufChar = (char*)txBuf;

#endif // RADIO_EN

int srData;
OneWire ds(PIN_ONE_WIRE);

void shift();
void set(int pin);
void clear(int pin);
void leds(bool tx, bool rx, bool err);
void readTemp(byte* data);
void floatToByte(byte* bytes, float f);
void debugWave(int waves);

void setup()
{
  pinMode(PIN_SR_LATCH, OUTPUT);
  pinMode(PIN_SR_CLOCK, OUTPUT);
  pinMode(PIN_SR_DATA, OUTPUT);

#if LED_DEBUG
  for (int i = 0; i < 2; i++) {
    leds(1, 1, 1);
    delay(FLASH_DELAY);
    
    leds(0, 0, 0);
    delay(FLASH_DELAY);
  }
#endif // LED_DEBUG

#if RADIO_EN
  if (!driver.init()) {
    leds(0, 0, 1);
    while(true) { }
  }
#endif // RADIO_EN

#if LED_DEBUG
  for (int i = 0; i < 2; i++) {
    leds(1, 1, 0);
    delay(FLASH_DELAY);
    
    leds(0, 0, 0);
    delay(FLASH_DELAY);
  }
#endif // LED_DEBUG
}

void loop() {
#if RADIO_EN
#if TX_TEST

  leds(1, 0, 0);

  sprintf(txBuf, "hello");
  driver.send(txBufChar, strlen(txBuf));
  driver.waitPacketSent();

  leds(0, 0, 0);

  delay(1000);

#else // not test

  leds(0, 1, 0);

  const char* helloMatch = "hello";
  const char* helloAck = "hello back ";
  const char* tempMatch = "temp";
  const char* tempAck = "temp back ";

  rxBuf[0] = NULL;
  uint8_t rxBufLen = sizeof(rxBuf);
  if (driver.recv(rxBuf, &rxBufLen)) {
    rxBuf[rxBufLen] = NULL;
    if (strstr(rxBufChar, helloMatch) != NULL) {
      const char number = rxBufChar[strlen(helloMatch) + 1];
      strcpy(txBufChar, helloAck);
      txBufChar[strlen(helloAck)] = number;
      txBufChar[strlen(helloAck) + 1] = NULL;
      
#if SR_DEBUG

      srData = '0' - number;
      shift();
      
#endif // SR_DEBUG

    }
    else if (strstr(rxBufChar, tempMatch) != NULL) {
      strcpy(txBufChar, tempAck);
      int i = strlen(tempAck);
      byte tempData[2];
      readTemp(tempData);
      txBufChar[i++] = tempData[0];
      txBufChar[i++] = tempData[1];
      txBufChar[i] = NULL;
    }
    else {
      strcpy(txBufChar, "error");
    }

#if TX_WAIT
    leds(0, 0, 0);
    delay(TX_WAIT_DELAY);
#endif //TX_WAIT

    leds(1, 0, 0);

    driver.send(txBuf, strlen(txBuf));
    driver.waitPacketSent();

    leds(0, 0, 0);
  }

#endif // TX_TEST

#else // no radio

#if LED_DEBUG
    leds(0, 0, 1);
    delay(FLASH_DELAY);
    
    leds(0, 0, 0);
    delay(FLASH_DELAY);
#endif // LED_DEBUG

#endif // RADIO_EN
}

void shift() {
  digitalWrite(PIN_SR_LATCH, LOW);
  shiftOut(PIN_SR_DATA, PIN_SR_CLOCK, MSBFIRST, srData);
  digitalWrite(PIN_SR_LATCH, HIGH);
}

void set(int pin) {
  bitSet(srData, pin);
}

void clear(int pin) {
  bitClear(srData, pin);
}

void leds(bool tx, bool rx, bool err)
{
#if LED_DEBUG
  
  if (tx) {
    set(SR_PIN_LED_TX);
  }
  else {
    clear(SR_PIN_LED_TX);
  }
  
  if (rx) {
    set(SR_PIN_LED_RX);
  }
  else {
    clear(SR_PIN_LED_RX);
  }
  
  if (err) {
    set(SR_PIN_LED_ERR);
  }
  else {
    clear(SR_PIN_LED_ERR);
  }
  
  shift();
  
#endif // LED_DEBUG
}

void debugWave(int waves) {
  for (int i = 0; i < waves; i++) {
    digitalWrite(PIN_DEBUG, HIGH);
    delay(10);
    digitalWrite(PIN_DEBUG, LOW);
    delay(10);
  }
}

void readTemp(byte* data) {
  ds.reset(); 
  ds.write(0xCC);
  ds.write(0x44);

  delay(750);

  ds.reset();
  ds.write(0xCC);
  ds.write(0xBE);

  data[0] = ds.read(); 
  data[1] = ds.read();
}
