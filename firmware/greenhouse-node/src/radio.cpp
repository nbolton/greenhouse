#include "radio.h"

#if RADIO_EN

#include <gh_protocol.h>

#include "leds.h"
#include "motor.h"
#include "pins.h"
#include "sr.h"
#include "temp.h"

// caution! compile RH_ASK.cpp with RH_ASK_ATTINY_USE_TIMER1 uncommented.
// otherwise TIMER0 will be used, and delay()/millis() stop working.
#include <RH_ASK.h>

#define GH_ADDR GH_ADDR_NODE_1  // unique to device

#define TX_BIT_RATE 2000
#define TX_WAIT_DELAY 20
#define ERROR_DELAY 500

RH_ASK driver(TX_BIT_RATE, PIN_RX, PIN_TX, PIN_TX_EN);
uint8_t rxBuf[GH_LENGTH];
uint8_t txBuf[GH_LENGTH];
byte sequence = 0;

bool handleRx();

void radio_init() {
  if (!driver.init()) {
    leds(0, 0, 1);
    while (true) {
      // ..
    }
  }
}

#if TX_TEST

void testRadio() {
  leds(1, 0, 0);

  sprintf(txBufChar, "hello");
  driver.send(txBuf, strlen(txBufChar));
  driver.waitPacketSent();

  leds(0, 0, 0);

  delay(1000);
}

#endif  // TX_TEST

void radio_loop() {
#if TX_TEST
  testRadio();
#else

  leds(0, 1, 0);

  uint8_t rxBufLen = GH_LENGTH;
  if (driver.recv(rxBuf, &rxBufLen)) {
    if (GH_TO(rxBuf) == GH_ADDR) {
      GH_TO(txBuf) = GH_FROM(rxBuf);
      GH_FROM(txBuf) = GH_ADDR;
      GH_SEQ(txBuf) = GH_SEQ(rxBuf);

      // by default, send commnad back to say what we're replying to.
      GH_CMD(txBuf) = GH_CMD_ACK;
      GH_DATA_1(txBuf) = GH_CMD(rxBuf);

      if (!handleRx()) {
        leds(0, 1, 1);
        delay(ERROR_DELAY);
      }

      delay(TX_WAIT_DELAY);
      leds(1, 0, 0);

      driver.send(txBuf, GH_LENGTH);
      driver.waitPacketSent();

      sequence = GH_SEQ(rxBuf);
    }
  }

#endif  // TX_TEST
}

bool isDupeRx() {
  // prevent duplicate messages, only in some cases.
  if (GH_SEQ(rxBuf) == sequence) {
    GH_CMD(txBuf) = GH_CMD_ERROR;
    GH_DATA_1(txBuf) = GH_ERROR_BAD_SEQ;
    return true;
  }
  return false;
}

bool handleRx() {
  switch (GH_CMD(rxBuf)) {
    
#if HELLO_EN

    case GH_CMD_HELLO: {
      // nothing to do; ack is the default.
    } break;

#endif  // HELLO_EN

#if TEMP_EN

    case GH_CMD_TEMP_DEVS_REQ: {
      GH_CMD(txBuf) = GH_CMD_TEMP_DEVS_RSP;
      GH_DATA_1(txBuf) = temp_devs();
    } break;

    case GH_CMD_TEMP_DATA_REQ: {
      GH_CMD(txBuf) = GH_CMD_TEMP_DATA_RSP;
      GH_DATA_1(txBuf) = temp_data(GH_DATA_1(rxBuf), 0);
      GH_DATA_2(txBuf) = temp_data(GH_DATA_1(rxBuf), 1);
    } break;

#endif  // TEMP_EN

#if MOTOR_EN

    case GH_CMD_MOTOR_SPEED: {
      motor_speed(GH_DATA_1(rxBuf));
    } break;

    case GH_CMD_MOTOR_RUN: {
      if (isDupeRx()) {
        return false;
      }
      if (!motor_run(GH_DATA_1(rxBuf), GH_DATA_2(rxBuf))) {
        GH_CMD(txBuf) = GH_CMD_ERROR;
        GH_DATA_1(txBuf) = GH_ERROR_BAD_MOTOR_CMD;
      }
    } break;

#endif  // MOTOR_EN

    default: {
      GH_CMD(txBuf) = GH_CMD_ERROR;
      GH_DATA_1(txBuf) = GH_ERROR_BAD_CMD;
      GH_DATA_2(txBuf) = GH_CMD(rxBuf);
      return false;
    };
  }

  // all good if we get here
  return true;
}

#endif  // RADIO_EN
