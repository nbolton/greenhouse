#if RADIO_EN

#include "Radio.h"

#include "ISystem.h"
#include <RH_ASK.h>

#define BIT_RATE 2000
#define RX_PIN D0
#define TX_PIN D4
#define PTT_PIN -1 // using SR instead; TODO: does -1 effectively disable this feature?
//#define DEBUG_LEDS
//#define RADIO_TRACE
//#define TOGGLE_MODE

#define SR_PIN_EN_TX 15
#define SR_PIN_LED_ERR 1
#define SR_PIN_LED_RX 5
#define SR_PIN_LED_TX 4

#define DELAY_RX 100
#define DELAY_TX 100
#define TIMEOUT_RX 2000
#define ERROR_PRINT_INTERVAL 10000
#define SIGNAL_SWITCH 1
#define TX_INTERVAL 5000

#ifndef RADIO_TRACE
#undef TRACE
#define TRACE(l)
#undef TRACE_F
#define TRACE_F(...)
#endif // RADIO_TRACE

namespace embedded {
namespace greenhouse {

RH_ASK s_driver(BIT_RATE, RX_PIN, TX_PIN, PTT_PIN);

int sequence = 0, i = 0, t = 0, lastPing = 0, lastZing = 0, recvOk = 0, rxErrors = 0, errorRate = 0,
    timeouts = 0;
Mode mode;
Signal s;
char modeString[10];
unsigned long rxStart;
unsigned long txLast;

Radio::Radio() : m_system(nullptr) {}

void Radio::Setup()
{
  if (!s_driver.init()) {
    TRACE("Radio ASK init failed");
    while (true) {
      // HCF
    }
  }

  setMode(Ping);
  setSignal((mode == Ping) || (mode == Zing) ? Tx : Rx);
}

void Radio::Loop()
{
  if (s == Tx) {
    tx();
  }
  else if (s == Rx) {
    rx();
  }
  else {
    if (millis() >= (txLast + TX_INTERVAL)) {
      setSignal(Tx);
    }
  }
}

bool Radio::Busy() { return s == Rx; }

void Radio::sr(int pin, bool set) { m_system->ShiftRegister(pin, set); }

void Radio::setSignal(Signal s_)
{
  s = s_;

#ifdef RADIO_TRACE
  const char *signalName;
  switch (s) {
  case Tx:
    signalName = "tx";
    break;

  case Rx:
    signalName = "rx";
    break;

  case Idle:
    signalName = "idle";
    break;

  default:
    signalName = "?";
    break;
  }
#endif // RADIO_TRACE

  TRACE_F("Radio signal: %s", signalName);

  if (s == Rx) {
    rxStart = millis();
  }
}

void Radio::setMode(Mode _mode)
{
  mode = _mode;
  switch (mode) {
  case Ping:
    strcpy(modeString, "ping >>");
    break;

  case Zing:
    strcpy(modeString, "zing >>");
    break;

  case Pong:
    strcpy(modeString, "<< pong");
    break;
  }
  TRACE_F("Radio mode: %s", modeString);
}

void Radio::errorFlash(int times)
{
#ifdef DEBUG_LEDS
  for (int i = 0; i < times; i++) {
    sr(SR_PIN_LED_ERR, true);
    delay(100);
    sr(SR_PIN_LED_ERR, false);
    delay(100);
  }
#endif
}

void Radio::toggleMode()
{
#ifdef TOGGLE_MODE
  // toggle between ping and zing
  setMode(mode == Ping ? Zing : Ping);
#endif
}

void Radio::tx()
{
  sr(SR_PIN_EN_TX, true);

  if (mode == Ping || mode == Pong) {
    sequence++;
  }

  char msg[50];
  sprintf(msg, "%s %d", modeString, sequence % 10);

  s_driver.send((uint8_t *)msg, strlen(msg));
  s_driver.waitPacketSent();

  TRACE_F("Radio sent: %s", msg);

  sr(SR_PIN_EN_TX, false);

#ifdef DEBUG_LEDS
  sr(SR_PIN_LED_TX, true);
  delay(100);
  sr(SR_PIN_LED_TX, false);
#endif

  delay(DELAY_TX);

#if SIGNAL_SWITCH
  setSignal(Rx);
#endif

  txLast = millis();
}

void Radio::rx()
{
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  if (s_driver.recv(buf, &buflen)) // Non-blocking
#pragma GCC diagnostic pop
  {
    rxStart = millis();

    // terminate string so garbage isn't printed
    buf[buflen] = '\0';
    TRACE_F("Radio recv: %s", (char *)buf);

    if (mode != Pong) {
      recvOk++;

      int *last = (mode == Ping) ? &lastPing : &lastZing;

      i = buf[buflen - 1] - 48;
      if ((*last != -1) && (i != *last + 1)) {
        int errorSince;
        if (i < *last) {
          errorSince = i - (*last - 9);
        }
        else {
          errorSince = i - *last - 1;
        }

        if (errorSince != 0) {
          rxErrors += errorSince;
          TRACE_F("Radio errors: now=%d, sum=%d, ok=%d", errorSince, rxErrors, recvOk);
          errorFlash(1);
        }
      }
      *last = i;

#ifdef DEBUG_LEDS
      sr(SR_PIN_LED_RX, true);
      delay(100);
      sr(SR_PIN_LED_RX, false);
#endif

      delay(DELAY_RX);

#if SIGNAL_SWITCH

      if ((mode == Ping) || (mode == Zing)) {
        toggleMode();
      }

      setSignal(Idle);
#endif
    }
  }

  int diff = millis() - t;
  if (diff > ERROR_PRINT_INTERVAL) {
    int errors = timeouts + rxErrors;
    errorRate = ((float)errors / (float)(recvOk + errors)) * 100;
    TRACE_F("Radio error rate: %d%% (ok=%d, err=%d)", errorRate, recvOk, errors);
    timeouts = 0;
    rxErrors = 0;
    recvOk = 0;
    t = millis();
  }

  if ((millis() - rxStart) > TIMEOUT_RX) {
    TRACE("Radio timeout");
    timeouts++;
    errorFlash(2);

    if ((mode == Ping) || (mode == Zing)) {
      toggleMode();

      // only tx if ping; if both tx they would get stuck trying to
      // talk to eachother at the same time.
      setSignal(Tx);
    }
    else {
      // restart timeout
      rxStart = millis();
    }
  }
}

} // namespace greenhouse
} // namespace embedded

#endif // RADIO_EN
