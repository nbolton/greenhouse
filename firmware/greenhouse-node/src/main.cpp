#include "attiny.h"

#define RADIO_EN 1
#define OW_EN 1

#define HELLO_EN 0
#define OW_SINGLE 0
#define OW_MAX_DEVS 4
#define SINGLE_BUF 1
#define LED_DEBUG 1
#define SR_DEBUG 0
#define TX_TEST 0
#define TX_BIT_RATE 2000
#define MOTOR_PWM 200
#define MOTOR_TEST 0

#if RADIO_EN
// caution! compile RH_ASK.cpp with RH_ASK_ATTINY_USE_TIMER1 uncommented.
// otherwise TIMER0 will be used, and delay()/millis() stop working.
#include <RH_ASK.h>
#endif // RADIO_EN

#if OW_EN

#include <OneWire.h>

#define OW_ADDR_LEN 8
#define OW_DELAY 750 // or 1000?
#define OW_DS18B20_CONVERT 0x44
#define OW_READ_SCRATCH 0xBE
#define OW_ALL_DEVS 0xCC // aka skip/ignore

#endif // OW_EN

#define FLASH_DELAY 200
#define TX_WAIT 1
#define TX_WAIT_DELAY 50
#define RX_BUF_LEN 8
#define TX_BUF_LEN 30

#define SR_EN 1
#define SR_TOTAL 1
#define SR_PIN_LED_ON 0
#define SR_PIN_LED_ERR 1
#define SR_PIN_LED_RX 2
#define SR_PIN_LED_TX 3
#define SR_PIN_MOTOR_B 4
#define SR_PIN_MOTOR_A 5

#define PIN_SR_OE PA7     // OE (13)
#define PIN_SR_DATA PA6   // SER (14)
#define PIN_SR_CLOCK PA5  // SRCLK (11)
#define PIN_SR_LATCH PA4  // RCLK (12)
#define PIN_TX_EN PA0
#define PIN_TX PA1
#define PIN_RX PA2
#define PIN_DEBUG PA1
#define PIN_ONE_WIRE PA3
#define PIN_MOTOR_PWM PB2

#define TEMP_UNKNOWN 255

#if RADIO_EN

RH_ASK driver(TX_BIT_RATE, PIN_RX, PIN_TX, PIN_TX_EN);

#if HELLO_EN
uint8_t sequence = 0;
#endif // HELLO_EN

#if SINGLE_BUF
uint8_t buf[TX_BUF_LEN];
#define rxBuf buf
#define txBuf buf
#else
uint8_t rxBuf[RX_BUF_LEN];
uint8_t txBuf[TX_BUF_LEN];
#endif // SINGLE_BUF

#endif // RADIO_EN

#if OW_SINGLE == 0 && OW_EN
byte owAddrs[OW_MAX_DEVS][OW_ADDR_LEN];
#endif

#if SR_EN
int srData = 0;
#endif // SR_EN

#if OW_EN
OneWire ow(PIN_ONE_WIRE);
#endif // OW_EN

void shift();
void set(int pin);
void clear(int pin);
void leds(bool tx, bool rx, bool err);
void floatToByte(byte* bytes, float f);
void debugWave(int waves);

#if OW_EN
#if OW_SINGLE
void readTemp(byte* data);
#else
uint8_t scanTempDevs();
void readTempDev(byte* addr, byte* data);
#endif // OW_SINGLE
#endif // OW_EN

void setup()
{
  analogWrite(PIN_MOTOR_PWM, MOTOR_PWM);

#if SR_EN
  digitalWrite(PIN_SR_OE, LOW);

  pinMode(PIN_SR_OE, OUTPUT);
  pinMode(PIN_SR_LATCH, OUTPUT);
  pinMode(PIN_SR_CLOCK, OUTPUT);
  pinMode(PIN_SR_DATA, OUTPUT);
#endif //SR_EN

#if LED_DEBUG
  set(SR_PIN_LED_ON);
  delay(FLASH_DELAY);
  leds(0, 0, 1);
  delay(FLASH_DELAY);
  leds(0, 1, 1);
  delay(FLASH_DELAY);
  leds(1, 1, 1);
  delay(FLASH_DELAY);
#endif // LED_DEBUG

#if RADIO_EN
  if (!driver.init()) {
    leds(0, 0, 1);
    while(true) { }
  }
#endif // RADIO_EN

#if LED_DEBUG
  for (int i = 0; i < 3; i++) {
    leds(1, 1, 0);
    delay(FLASH_DELAY);
    
    clear(SR_PIN_LED_ON);
    leds(0, 0, 0);
    delay(FLASH_DELAY);
  }
  shift();
#endif // LED_DEBUG
}

void loop() {

#if MOTOR_TEST

const int pwmFrom = 100;//(255.0f / 0.9f);
const int pwmTo = 255;
const int pwmDelta = 10;
int d = 2000;
int pwm = pwmFrom;
bool forward = true;

while (true) {

  analogWrite(PIN_MOTOR_PWM, pwm);

  clear(SR_PIN_MOTOR_B);
  set(SR_PIN_MOTOR_A);
  shift();
  delay(d);
  
  clear(SR_PIN_MOTOR_A);
  shift();
  delay(d);

  clear(SR_PIN_MOTOR_A);
  set(SR_PIN_MOTOR_B);
  shift();
  delay(d);
  
  clear(SR_PIN_MOTOR_B);
  shift();
  delay(d);

  if (forward) {
    pwm += pwmDelta;
  }
  else {
    pwm -= pwmDelta;
  }

  if (pwm >= pwmTo) {
    pwm = pwmTo;
    forward = false;
  }

  if (pwm <= pwmFrom) {
    break;
  }

}

return;
#endif // MOTOR_TEST

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

#if HELLO_EN
  const char* helloMatch = "hello";
  const char* helloAck = "hello back ";
#endif // HELLO_EN

  const char* tempMatch = "t>";
  const char* tempAck = "t<";
  const char* motorMatch = "m>";
  const char* motorAck = "m<";
  
#if SINGLE_BUF
char* rxBufChar = (char*)buf;
char* txBufChar = (char*)buf;
#else
char* rxBufChar = (char*)rxBuf;
char* txBufChar = (char*)txBuf;
#endif // SINGLE_BUF

  rxBuf[0] = NULL;
  uint8_t rxBufLen = sizeof(rxBuf);
  uint8_t txBufLen = 0;
  if (driver.recv(rxBuf, &rxBufLen)) {
    rxBuf[rxBufLen] = NULL;
#if HELLO_EN
    if (strstr(rxBufChar, helloMatch) != NULL) {
      const char number = rxBufChar[strlen(helloMatch) + 1];
      strcpy(txBufChar, helloAck);
      txBufChar[strlen(helloAck)] = number;
      txBufChar[strlen(helloAck) + 1] = NULL;
      txBufLen = strlen(txBufChar);
      
#if SR_DEBUG

      srData = '0' - number;
      shift();
      
#endif // SR_DEBUG

    }
    else 
#endif // HELLO_EN
    if (strstr(rxBufChar, tempMatch) != NULL) {
      
      strcpy(txBufChar, tempAck);
      int i = strlen(tempAck);

      byte tempData[2];
#if OW_SINGLE
      readTemp(tempData);
      txBufChar[i++] = 1;
      txBufChar[i++] = tempData[0];
      txBufChar[i++] = tempData[1];
#else
      uint8_t devs = scanTempDevs();
      txBufChar[i++] = devs;
      for (int j = 0; j < devs; j++) {
        readTempDev(owAddrs[j], tempData);
        txBufChar[i++] = tempData[0];
        txBufChar[i++] = tempData[1];
      }
#endif

      txBufLen = i;
    }
    else if (strstr(rxBufChar, motorMatch) != NULL) {
      
      bool ok = true;
      const char d = rxBufChar[rxBufLen - 1];
      switch (d) {
        case 'b': {
          set(SR_PIN_MOTOR_A);
          clear(SR_PIN_MOTOR_B);
          shift();
        }
        break;
        case 'f': {
          set(SR_PIN_MOTOR_B);
          clear(SR_PIN_MOTOR_A);
          shift();
        }
        break;
        case 's': {
          clear(SR_PIN_MOTOR_B);
          clear(SR_PIN_MOTOR_A);
          shift();
        }
        break;
        default: {
          ok = false;
          strcpy(txBufChar, "invalid");
          txBufLen = strlen(txBufChar);
        }
        break;
      }

      if (ok) {
        strcpy(txBufChar, motorAck);
        txBufLen = strlen(motorAck);
      }
    }
    else {
      const char* error = "error";
      strcpy(txBufChar, error);
      txBufLen = strlen(error);
    }

#if TX_WAIT
    leds(0, 0, 0);
    delay(TX_WAIT_DELAY);
#endif //TX_WAIT

    leds(1, 0, 0);

    driver.send(txBuf, txBufLen);
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

#if SR_EN
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
#else
void shift() {
}

void set(int pin) {
}

void clear(int pin) {
}
#endif

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

#if SR_DEBUG
void debugWave(int waves) {
  for (int i = 0; i < waves; i++) {
    digitalWrite(PIN_DEBUG, HIGH);
    delay(10);
    digitalWrite(PIN_DEBUG, LOW);
    delay(10);
  }
}
#endif // SR_DEBUG

#if OW_EN
#if OW_SINGLE

// only supports 1 device on the wire
void readTemp(byte* data) {
  ow.reset(); 
  ow.write(OW_ALL_DEVS);
  ow.write(OW_DS18B20_CONVERT);
  delay(750);
  ow.reset();
  ow.write(OW_ALL_DEVS);
  ow.write(OW_READ_SCRATCH);
  data[0] = ow.read(); 
  data[1] = ow.read();
}

#else // many devs

uint8_t scanTempDevs() {
  uint8_t devs;
  for (devs = 0; devs < OW_MAX_DEVS; devs++) {
    if (!ow.search(owAddrs[devs])) {
      break;
    }
  }
  ow.reset_search();
  return devs;
}

void readTempDev(byte* addr, byte* data) {
  
  data[0] = TEMP_UNKNOWN;
  data[1] = TEMP_UNKNOWN;

  // tell DS18B20 to take a temperature reading and put it on the scratchpad.
  if (!ow.reset()) {
    return;
  }
  ow.select(addr);
  ow.write(OW_DS18B20_CONVERT);

  // wait for DS18B20 to do it's thing.
  delay(OW_DELAY);
  
  // tell DS18B20 to read from it's scratchpad.
  if (!ow.reset()) {
    return;
  }
  ow.select(addr);
  ow.write(OW_READ_SCRATCH);
  
  data[0] = ow.read();
  data[1] = ow.read();

  // HACK: ignore the rest of the data
  delay(100);
}
#endif // OW_EN

#endif
