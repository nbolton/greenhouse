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

#define TX_WAIT 1
#define TX_WAIT_DELAY 20
#define TX_BIT_RATE 2000

RH_ASK driver(TX_BIT_RATE, PIN_RX, PIN_TX, PIN_TX_EN);
uint8_t rxBuf[GH_LENGTH];
uint8_t txBuf[GH_LENGTH];

void handle();

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
      handle();
    } else {
      leds(0, 1, 1);
    }
  }

#endif  // TX_TEST
}

void handle() {
  GH_TO(txBuf) = GH_FROM(rxBuf);
  GH_FROM(txBuf) = GH_ADDR;
  GH_CMD(txBuf) = GH_CMD_ACK;
  GH_DATA_1(txBuf) = GH_CMD(rxBuf);

  switch (GH_CMD(rxBuf)) {
#if HELLO_EN
    case GH_CMD_HELLO: {
      // send commnad back to say what we're replying to,
      // and send the same number back so sequence can be checked.
      GH_DATA_2(txBuf) = GH_DATA_1(rxBuf);
    } break;
#endif  // HELLO_EN

#if TEMP_EN

    case GH_CMD_TEMP_DEVS_REQ: {
      GH_CMD(txBuf) = GH_CMD_TEMP_DEVS_RSP;
#if OW_SINGLE
      GH_DATA_1(txBuf) = 1;
#else
      GH_DATA_1(txBuf) = scanTempDevs();
#endif  // OW_SINGLE
    } break;

    case GH_CMD_TEMP_DATA_REQ: {
      GH_CMD(txBuf) = GH_CMD_TEMP_DATA_RSP;
#if OW_SINGLE
      readTemp();
#else
      readTempDev(GH_DATA_1(rxBuf));
#endif  // OW_SINGLE
      GH_DATA_1(txBuf) = tempData(0);
      GH_DATA_2(txBuf) = tempData(1);
    } break;

#endif  // TEMP_EN

#if MOTOR_EN

    case GH_CMD_MOTOR_RUN: {
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
    } break;
  }

#if TX_WAIT
  leds(0, 0, 0);
  delay(TX_WAIT_DELAY);
#endif  // TX_WAIT

  leds(1, 0, 0);

  driver.send(txBuf, GH_LENGTH);
  driver.waitPacketSent();
}

#endif  // RADIO_EN