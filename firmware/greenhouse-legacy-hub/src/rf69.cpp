#include "rf69.h"

#include "common.h"
#include "hc12.h"
#include "rf95.h"

#include <Arduino.h>
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <radio_common.h>

#define PIN_IRQ PA2
#define PIN_CS PA1

static RH_RF69 s_rf69(PIN_CS, PIN_IRQ);
static RHReliableDatagram s_rd(s_rf69, RHRD_ADDR_RELAY);

using namespace greenhouse::radio;

namespace rf69 {

void rx();

void init()
{
  if (!s_rd.init()) {
    halt();
  }

  if (!s_rf69.setFrequency(868.0)) {
    halt();
  }

  s_rf69.setTxPower(20, true);
}

void update() { rx(); }

void relay(const char *source, char *relayData, int length)
{
  char dataChar[RH_MAX_MESSAGE_LEN];
  uint8_t *data = (uint8_t *)&dataChar;

  sprintf(dataChar, "%s:", source);
  int preLen = strlen(dataChar);

  const char *relayBuf = dataChar + preLen;
  memcpy((void *)relayBuf, relayData, length);

  s_rf69.send(data, RH_MAX_MESSAGE_LEN);
  s_rf69.waitPacketSent();
}

void rx(MessageType type, int dataLen, char *data) {}

void rx()
{
  if (s_rf69.available()) {
    uint8_t rxBuf[RH_MAX_MESSAGE_LEN];
    uint8_t rxLen = sizeof(rxBuf);

    uint8_t txBuf[RH_MAX_MESSAGE_LEN];
    uint8_t txLen = sizeof(rxBuf);
    char *txBufCh = (char *)txBuf;

    if (s_rf69.recv(rxBuf, &rxLen)) {

      char *bufChar = (char *)rxBuf;
      if (strstr(bufChar, RADIO_MSG_PRE RADIO_MSG_SEP_STR) != nullptr) {

        char typeData, dataLenData;
        sscanf(bufChar, RADIO_MSG_PROTO, &typeData, &dataLenData);
        char *c1 = strchr(bufChar, RADIO_MSG_SEP_CHAR);
        char *c2 = strchr(c1 + 1, RADIO_MSG_SEP_CHAR);
        char *innerData = c2 + 1;

        MessageType type = (MessageType)typeData;
        int dataLen = (int)dataLenData;

        rx(type, dataLen, innerData);
      }
      else {
        // unrecognized
      }
    }
  }
}

} // namespace rf69
