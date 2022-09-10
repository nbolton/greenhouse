#include "Radio.h"

#include "ISystem.h"
#include <RH_ASK.h>

#define BIT_RATE 2000
#define RX_PIN D0
#define TX_PIN D4
#define PTT_PIN 0 // using SR instead

#define SR_PIN_EN_TX 15
#define SR_PIN_LED_ERR 1
#define SR_PIN_LED_RX 5
#define SR_PIN_LED_TX 4

#define DELAY_RX 100
#define DELAY_TX 100
#define TIMEOUT_RX 2000
#define ERROR_PRINT_INTERVAL 10000
#define SIGNAL_SWITCH 1

namespace embedded {
namespace greenhouse {

enum Mode {
  Ping,
  Zing,
  Pong,
};

RH_ASK s_driver(BIT_RATE, RX_PIN, TX_PIN, PTT_PIN);

int sequence = 0, i = 0, t = 0, last = 0, recvOk = 0, rxErrors = 0, errorRate = 0, timeouts = 0;
Mode mode;
Signal s;
char modeString[10];
unsigned long rxStart;

Radio::Radio() : m_log(), m_system(nullptr) {}

void Radio::Setup()
{
  if (!s_driver.init()) {
    Log().Trace(F("RadioHead ASK init failed"));
    while (true) {
      // HCF
    }
  }

  mode = Pong;
  Serial.print("mode: ");
  sprintf(modeString, "%s", mode == Ping ? "ping >>" : "<< pong");
  Serial.println(modeString);

  setSignal((mode == Ping) ? Tx : Rx);
}

void Radio::Loop()
{
  if (s == Tx) {
    tx();
  }
  else {
    rx();
  }
}

bool Radio::Busy() { return s == Rx; }

void Radio::sr(int pin, bool set) { m_system->ShiftRegister(pin, set); }

void Radio::setSignal(Signal s_)
{
  s = s_;
  Serial.print("signal: ");
  Serial.println(s == Tx ? "tx" : "rx");

  if (s == Rx) {
    rxStart = millis();
  }
}

void Radio::errorFlash(int errorFlash)
{
  sr(SR_PIN_LED_ERR, true);
  delay(100);
  sr(SR_PIN_LED_ERR, false);
  delay(100);
  sr(SR_PIN_LED_ERR, true);
  delay(100);
  sr(SR_PIN_LED_ERR, false);
}

void Radio::tx()
{
  sr(SR_PIN_EN_TX, true);

  char msg[50];
  sprintf(msg, "%s %d", modeString, sequence++ % 10);

  s_driver.send((uint8_t *)msg, strlen(msg));
  s_driver.waitPacketSent();

  Serial.print("sent: ");
  Serial.println(msg);

  sr(SR_PIN_EN_TX, false);

  sr(SR_PIN_LED_TX, true);
  delay(100);
  sr(SR_PIN_LED_TX, false);

  delay(DELAY_TX);

#if SIGNAL_SWITCH
  setSignal(Rx);
#endif
}

//#define WAIT_STR "zing"
#define WAIT_STR "ping"

void Radio::rx()
{
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

  if (s_driver.recv(buf, &buflen)) // Non-blocking
  {
    rxStart = millis();

    // terminate string so garbage isn't printed
    buf[buflen] = '\0';
    Serial.print("recv: ");
    Serial.println((char *)buf);

    if (strstr((char *)buf, WAIT_STR) != NULL) {
      recvOk++;
      Serial.println(WAIT_STR "!");

      i = buf[buflen - 1] - 48;
      if ((last != -1) && (i != last + 1)) {
        int errorSince;
        if (i < last) {
          errorSince = i - (last - 9);
        }
        else {
          errorSince = i - last - 1;
        }

        if (errorSince != 0) {
          rxErrors += errorSince;
          Serial.print("! errors: now=");
          Serial.print(errorSince);
          Serial.print(", sum=");
          Serial.print(rxErrors);
          Serial.print(", ");
          Serial.print("ok=");
          Serial.print(recvOk);
          Serial.println();

          errorFlash(1);
        }
      }
      last = i;

      sr(SR_PIN_LED_RX, true);
      delay(100);
      sr(SR_PIN_LED_RX, false);

      delay(DELAY_RX);

#if SIGNAL_SWITCH
      setSignal(Tx);
#endif
    }
  }

  int diff = millis() - t;
  if (diff > ERROR_PRINT_INTERVAL) {
    int errors = timeouts + rxErrors;
    errorRate = ((float)errors / (float)(recvOk + errors)) * 100;
    Serial.println("");
    Serial.print("error rate: ");
    Serial.print(errorRate);
    Serial.print("% (ok=");
    Serial.print(recvOk);
    Serial.print(", ");
    Serial.print("err=");
    Serial.print(errors);
    Serial.print(")");
    Serial.println("\n");

    timeouts = 0;
    rxErrors = 0;
    recvOk = 0;
    t = millis();
  }

  if ((millis() - rxStart) > TIMEOUT_RX) {
    Serial.println("timeout");
    timeouts++;
    errorFlash(2);

    if (mode == Ping) {
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
