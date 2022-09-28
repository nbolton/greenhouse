#include <Arduino.h>
#include <RH_ASK.h>
#include <gh_protocol.h>

#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h>  // Not actually used but needed to compile
#endif

#include "pins.h"
#include "sr.h"

#define HELLO_EN 1
#define TEMP_EN 1
#define MOTOR_EN 1

#define TIMEOUT 2000
#define TX_BIT_RATE 2000
#define TX_WAIT_DELAY 10
#define TX_RETRY_MAX 5
#define FLASH_DELAY 200
#define TEMP_SINGLE 0
#define LOOP_DELAY 100
#define START_DELAY 500
#define SERIAL_DELAY 500

#define SR_PIN_EN_TX 0
#define SR_PIN_LED_ERR 1
#define SR_PIN_LED_RX 2
#define SR_PIN_LED_TX 3

#define TEMP_OFFSET -1.2
#define TEMP_UNKNOWN 255
#define TEMP_DEVS_MAX 4

#define MOTOR_DELAY 3000  // millis
#define MOTOR_RUNTIME 2   // seconds

enum ErrorType {
  k_ErrorTimeout,  // gave up waiting for response
  k_ErrorInvalid,  // got a response, but was unexpected
  k_ErrorRemote,   // got a response, error at other side
};

struct TempData {
  int devs;
  float temps[TEMP_DEVS_MAX];
};

RH_ASK driver(TX_BIT_RATE, PIN_RX, PIN_TX, PIN_TX_EN);
uint8_t rxBuf[GH_LENGTH];
uint8_t txBuf[GH_LENGTH];
uint8_t sequence = 1;  // start at 1; reciever starts at 0
int cmd = 0;
int errors = 0;

bool sendHelloReq();
bool sendTempReq(TempData* data);
bool tempReqRetry();
bool sendMotorReq(byte dir, byte sec);
bool motorReqRetry(byte dir, byte sec);
void errorFlash(ErrorType type);
void printBuffer(const __FlashStringHelper* prompt, const uint8_t* buf,
                 uint8_t len);
void incrementSeq();
bool helloReqRetry();
bool sendTempDevsReq(TempData* data);
bool sendTempDataReq(int dev, TempData* data);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    delay(1);
  }
  delay(SERIAL_DELAY);

  pinMode(PIN_TX_EN, INPUT);

  Serial.println(F("\n"));
  Serial.println(F("ask tx/rx @ ") + String(TX_BIT_RATE) +
                 F("bps, esp8266 request temp with ask"));

  digitalWrite(PIN_SR_LATCH, LOW);
  digitalWrite(PIN_SR_CLOCK, LOW);
  digitalWrite(PIN_SR_DATA, LOW);

  pinMode(PIN_SR_LATCH, OUTPUT);
  pinMode(PIN_SR_CLOCK, OUTPUT);
  pinMode(PIN_SR_DATA, OUTPUT);

  set(SR_PIN_LED_ERR);
  set(SR_PIN_LED_RX);
  set(SR_PIN_LED_TX);
  shift();
  delay(START_DELAY);

  Serial.print(F("init radio driver... "));
  if (!driver.init()) {
    Serial.println(F("failed"));
    set_shift(SR_PIN_LED_ERR);
    while (true) {
      delay(1);
    }
  }
  Serial.println(F("ok"));

  clear(SR_PIN_LED_ERR);
  clear(SR_PIN_LED_RX);
  clear(SR_PIN_LED_TX);
  shift();

  for (int i = 0; i < 2; i++) {
    set(SR_PIN_LED_RX);
    set(SR_PIN_LED_TX);
    shift();
    delay(FLASH_DELAY);

    clear(SR_PIN_LED_RX);
    clear(SR_PIN_LED_TX);
    shift();
    delay(FLASH_DELAY);
  }

  Serial.println(F("running"));
  Serial.println(F("commands: htfrs"));
}

void loop() {
  if (Serial.available() > 0) {
    bool ok = false;
    const char c = Serial.read();
    Serial.read();
    switch (c) {
#if HELLO_EN

      case 'h':
        Serial.println(F("hello"));
        ok = helloReqRetry();
        break;

#endif  // HELLO_EN

#if TEMP_EN

      case 't':
        ok = tempReqRetry();
        break;

#endif  // TEMP_EN

#if MOTOR_EN

      case 'f':
        Serial.println(F("motor forward"));
        ok = motorReqRetry(GH_MOTOR_FORWARD, MOTOR_RUNTIME);
        break;

      case 'r':
        Serial.println(F("motor reverse"));
        ok = motorReqRetry(GH_MOTOR_REVERSE, MOTOR_RUNTIME);
        break;

#endif  // MOTOR_EN

      default:
        Serial.println(F("unknown: ") + String(c));
        break;
    }
    Serial.println(ok ? F("request ok") : F("request failed"));
    Serial.println(F("total errors: ") + String(errors));
    Serial.println(F("----"));
  } else {
    bool ok = false;
    bool check = true;
    switch (cmd++) {
      case 0: {
#if HELLO_EN
        ok = helloReqRetry();
#else
        check = false;
#endif  // HELLO_EN
      } break;

      case 1: {
#if TEMP_EN
        ok = tempReqRetry();
#else
        check = false;
#endif  // TEMP_EN
      } break;

#if MOTOR_EN

      case 2: {
        Serial.println(F("motor forward"));
        ok = motorReqRetry(GH_MOTOR_FORWARD, MOTOR_RUNTIME);
        delay(MOTOR_DELAY);
      } break;

      case 3: {
        Serial.println(F("motor reverse"));
        ok = motorReqRetry(GH_MOTOR_REVERSE, MOTOR_RUNTIME);
        delay(MOTOR_DELAY);
      } break;

#endif  // MOTOR_EN

      default:
        cmd = 0;
        check = false;
        break;
    }

    if (check) {
      Serial.println(ok ? F("request ok") : F("request failed"));
      Serial.println(F("total errors: ") + String(errors));
      Serial.println(F("----"));
    }
  }

  delay(LOOP_DELAY);
}

#if HELLO_EN

bool helloReqRetry() {
  Serial.println(F("saying hello"));
  bool helloBack = false;
  while (true) {
    helloBack = sendHelloReq();
    if (helloBack) {
      Serial.println(F("got hello back"));
      break;
    } else {
      Serial.println(F("retry hello req"));
    }
  }
  incrementSeq();
  return helloBack;
}

bool sendHelloReq() {
  Serial.println(F("[> tx] sending hello req"));

  set(SR_PIN_EN_TX);
  set(SR_PIN_LED_TX);
  shift();
  delay(TX_WAIT_DELAY);

  GH_TO(txBuf) = GH_ADDR_NODE_1;
  GH_FROM(txBuf) = GH_ADDR_MAIN;
  GH_CMD(txBuf) = GH_CMD_HELLO;

  sequence = sequence % 256;
  GH_SEQ(txBuf) = sequence;

  driver.send(txBuf, GH_LENGTH);
  driver.waitPacketSent();
  printBuffer(F("[> tx] sent: "), txBuf, GH_LENGTH);

  clear(SR_PIN_EN_TX);
  clear(SR_PIN_LED_TX);
  set(SR_PIN_LED_RX);
  shift();

  bool ackOk = false;
  unsigned long start = millis();
  bool rx = false;
  while (millis() < (start + TIMEOUT)) {
    uint8_t rxBufLen = GH_LENGTH;
    if (driver.recv(rxBuf, &rxBufLen)) {
      printBuffer(F("[< rx] got: "), rxBuf, GH_LENGTH);
      if (GH_TO(rxBuf) == GH_ADDR_MAIN) {
        rx = true;

        if (GH_CMD(rxBuf) == GH_CMD_ERROR) {
          Serial.println(F("[< rx] error code: ") + String(GH_DATA_1(rxBuf)));
          errorFlash(k_ErrorRemote);
        } else if (GH_CMD(rxBuf) != GH_CMD_ACK) {
          Serial.println(F("[< rx] error, invalid command"));
          errorFlash(k_ErrorInvalid);
        } else {
          rx = true;
          const int rxSeq = GH_SEQ(rxBuf);
          if (rxSeq != sequence) {
            Serial.println(F("[< rx] error, invalid ack: ") + String(rxSeq) +
                           F("!=") + String(sequence));
            errorFlash(k_ErrorInvalid);
          } else {
            ackOk = true;
          }
        }
        break;
      } else {
        Serial.println(F("[< rx] ignoring (address mismatch)"));
      }
    }
    delay(1);  // prevent WDT reset
  }

  if (!rx) {
    Serial.println(F("[< rx] error, timeout"));
    errorFlash(k_ErrorTimeout);
  }

  clear_shift(SR_PIN_LED_ERR);
  clear_shift(SR_PIN_LED_RX);

  return ackOk;
}

#endif  // HELLO_EN

#if TEMP_EN

bool tempReqRetry() {
  TempData tempData;
  bool ok = false;
  Serial.println(F("getting temp"));
  for (int i = 0; i < TX_RETRY_MAX; i++) {
#if TEMP_SINGLE
    tempData.devs = 1;
    ok = sendTempDataReq(0, &tempData);
#else
    ok = sendTempDevsReq(&tempData);
#endif  // TEMP_SINGLE

    if (ok) {
      break;
    }

    Serial.println(F("retry temp req"));
  }

  incrementSeq();

  if (ok) {
    for (int dev = 0; dev < tempData.devs; dev++) {
      for (int retry = 0; retry < TX_RETRY_MAX; retry++) {
        if (sendTempDataReq(dev, &tempData)) {
          break;
        }
        Serial.println(F("[< rx] retry temp data req"));
      }

      incrementSeq();
    }

    Serial.println(F("temp devs: ") + String(tempData.devs));

    if (tempData.devs == 0) {
      Serial.println(F("[< rx] no temp devs"));
      errorFlash(k_ErrorInvalid);
    } else {
      for (int i = 0; i < tempData.devs; i++) {
        float temp = tempData.temps[i];
        Serial.print(F("temp[") + String(i) + F("] is "));
        if (temp != TEMP_UNKNOWN) {
          temp += TEMP_OFFSET;
          Serial.println(String(temp) + F("°C (calibrated)"));
        } else {
          Serial.println(F("unknown"));
          errorFlash(k_ErrorInvalid);
        }
      }
    }
  }

  return ok;
}

bool sendTempDevsReq(TempData* data) {
  Serial.println(F("[> tx] sending temp devs req"));

  set(SR_PIN_EN_TX);
  set(SR_PIN_LED_TX);
  shift();
  delay(TX_WAIT_DELAY);

  GH_TO(txBuf) = GH_ADDR_NODE_1;
  GH_FROM(txBuf) = GH_ADDR_MAIN;
  GH_CMD(txBuf) = GH_CMD_TEMP_DEVS_REQ;

  sequence = sequence % 256;
  GH_SEQ(txBuf) = sequence;

  driver.send(txBuf, GH_LENGTH);
  driver.waitPacketSent();
  printBuffer(F("[> tx] sent: "), txBuf, GH_LENGTH);

  clear(SR_PIN_EN_TX);
  clear(SR_PIN_LED_TX);
  set(SR_PIN_LED_RX);
  shift();

  data->devs = 0;

  unsigned long start = millis();
  bool rx = false;
  while (millis() < (start + TIMEOUT)) {
    uint8_t rxBufLen = GH_LENGTH;
    if (driver.recv(rxBuf, &rxBufLen)) {
      printBuffer(F("[< rx] got: "), rxBuf, GH_LENGTH);

      if (GH_TO(rxBuf) == GH_ADDR_MAIN) {
        rx = true;

        if (GH_CMD(rxBuf) == GH_CMD_ERROR) {
          Serial.println(F("[< rx] error code: ") + String(GH_DATA_1(rxBuf)));
          errorFlash(k_ErrorRemote);
        } else if (GH_CMD(rxBuf) != GH_CMD_TEMP_DEVS_RSP) {
          Serial.println(F("[< rx] error, unexpected command"));
          errorFlash(k_ErrorInvalid);
        } else {
          data->devs = GH_DATA_1(rxBuf);
          Serial.println(F("[< rx] temp devs: ") + String(data->devs));
        }

        break;
      } else {
        Serial.println(F("[< rx] ignoring (address mismatch)"));
      }
    }
    delay(1);  // prevent WDT reset
  }

  if (!rx) {
    Serial.println(F("[< rx] error, timeout"));
    errorFlash(k_ErrorTimeout);
  }

  clear_shift(SR_PIN_LED_ERR);
  clear_shift(SR_PIN_LED_RX);

  return rx;
}

bool sendTempDataReq(int dev, TempData* data) {
  data->temps[dev] = TEMP_UNKNOWN;
  Serial.println(F("[> tx] sending temp data req: ") + String(dev));

  set(SR_PIN_EN_TX);
  set(SR_PIN_LED_TX);
  shift();
  delay(TX_WAIT_DELAY);

  GH_TO(txBuf) = GH_ADDR_NODE_1;
  GH_FROM(txBuf) = GH_ADDR_MAIN;
  GH_CMD(txBuf) = GH_CMD_TEMP_DATA_REQ;
  GH_DATA_1(txBuf) = dev;

  sequence = sequence % 256;
  GH_SEQ(txBuf) = sequence;

  driver.send(txBuf, GH_LENGTH);
  driver.waitPacketSent();
  printBuffer(F("[> tx] sent: "), txBuf, GH_LENGTH);

  clear(SR_PIN_EN_TX);
  clear(SR_PIN_LED_TX);
  set(SR_PIN_LED_RX);
  shift();

  unsigned long start = millis();
  bool rx = false;
  while (millis() < (start + TIMEOUT)) {
    uint8_t rxBufLen = GH_LENGTH;
    if (driver.recv(rxBuf, &rxBufLen)) {
      printBuffer(F("[< rx] got: "), rxBuf, GH_LENGTH);

      if (GH_TO(rxBuf) == GH_ADDR_MAIN) {
        rx = true;

        if (GH_CMD(rxBuf) == GH_CMD_ERROR) {
          Serial.println(F("[< rx] error code: ") + String(GH_DATA_1(rxBuf)));
          errorFlash(k_ErrorRemote);
        } else if (GH_CMD(rxBuf) != GH_CMD_TEMP_DATA_RSP) {
          Serial.println(F("[< rx] error, unexpected command"));
          errorFlash(k_ErrorInvalid);
        } else {
          const uint8_t a = GH_DATA_1(rxBuf);
          const uint8_t b = GH_DATA_2(rxBuf);
          Serial.println(F("[< rx] float a=") + String(a) + F(" b=") +
                         String(b));

          if ((a != TEMP_UNKNOWN) && (b != TEMP_UNKNOWN)) {
            data->temps[dev] = b * 16.0 + a / 16.0;
          }
        }

        break;
      } else {
        Serial.println(F("[< rx] ignoring (address mismatch)"));
        // don't report an error, this is fine
      }
    }
    delay(1);  // prevent WDT reset
  }

  if (!rx) {
    Serial.println(F("[< rx] error, timeout"));
    errorFlash(k_ErrorTimeout);
  }

  clear_shift(SR_PIN_LED_ERR);
  clear_shift(SR_PIN_LED_RX);

  return rx;
}

#endif  // TEMP_EN

#if MOTOR_EN

bool motorReqRetry(byte dir, byte sec) {
  bool ok = false;
  Serial.println(F("motor command"));
  for (int i = 0; i < TX_RETRY_MAX; i++) {
    ok = sendMotorReq(dir, sec);
    if (ok) {
      break;
    }
    Serial.println(F("retry motor req"));
  }

  incrementSeq();
  return ok;
}

bool sendMotorReq(byte dir, byte sec) {
  set(SR_PIN_EN_TX);
  set(SR_PIN_LED_TX);
  shift();
  delay(TX_WAIT_DELAY);

  GH_TO(txBuf) = GH_ADDR_NODE_1;
  GH_FROM(txBuf) = GH_ADDR_MAIN;
  GH_CMD(txBuf) = GH_CMD_MOTOR_RUN;
  GH_DATA_1(txBuf) = dir;
  GH_DATA_2(txBuf) = sec;

  sequence = sequence % 256;
  GH_SEQ(txBuf) = sequence;

  driver.send(txBuf, GH_LENGTH);
  driver.waitPacketSent();
  printBuffer(F("[> tx] sent: "), txBuf, GH_LENGTH);

  clear(SR_PIN_EN_TX);
  clear(SR_PIN_LED_TX);
  set(SR_PIN_LED_RX);
  shift();

  unsigned long start = millis();
  bool rx = false;
  while (millis() < (start + TIMEOUT)) {
    uint8_t rxBufLen = sizeof(rxBuf);
    if (driver.recv(rxBuf, &rxBufLen)) {
      printBuffer(F("[< rx] got: "), txBuf, GH_LENGTH);

      if (GH_TO(rxBuf) == GH_ADDR_MAIN) {
        rx = true;

        if (GH_CMD(rxBuf) == GH_CMD_ERROR) {
          Serial.println(F("[< rx] error code: ") + String(GH_DATA_1(rxBuf)));
          errorFlash(k_ErrorRemote);
        } else if (GH_CMD(rxBuf) != GH_CMD_ACK) {
          Serial.println(F("[< rx] error, unexpected command"));
          errorFlash(k_ErrorInvalid);
        }

        break;
      } else {
        Serial.println(F("[< rx] ignoring (address mismatch)"));
        // don't report an error, this is fine
      }
    }
    delay(1);  // prevent WDT reset
  }

  if (!rx) {
    Serial.println(F("[< rx] error, timeout"));
    errorFlash(k_ErrorTimeout);
  }

  clear_shift(SR_PIN_LED_ERR);
  clear_shift(SR_PIN_LED_RX);

  return rx;
}

#endif  // MOTOR_EN

void errorFlash(ErrorType type) {
  errors++;
  for (int i = 0; i < type; i++) {
    set_shift(SR_PIN_LED_ERR);
    delay(FLASH_DELAY);
    clear_shift(SR_PIN_LED_ERR);
    delay(FLASH_DELAY);
  }
}

void printBuffer(const __FlashStringHelper* prompt, const uint8_t* buf,
                 uint8_t len) {
  Serial.print(prompt);
  for (uint8_t i = 0; i < len; i++) {
    if (i % 16 == 15) {
      Serial.println(buf[i], HEX);
    } else {
      Serial.print(buf[i], HEX);
      Serial.print(F(" "));
    }
  }
  Serial.println(F(""));
}

void incrementSeq() {
  if (sequence == 255) {
    sequence = 0;
  } else {
    sequence++;
  }
}
