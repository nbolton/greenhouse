#include "radio_common.h"

#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <trace.h>

#define PRINT_BUFFER_LEN 200

RHReliableDatagram *p_rd;
RH_RF69 *p_rf69;

namespace greenhouse {
namespace radio {
namespace common {

RxCallback rxCallback;
StateCallback stateCallback;
int txFailures = 0;

void init(
  ::RHReliableDatagram *rd, ::RH_RF69 *rf69, RxCallback _rxCallback, StateCallback _stateCallback)
{
  p_rd = rd;
  p_rf69 = rf69;
  rxCallback = _rxCallback;
  stateCallback = _stateCallback;
}

bool tx(Message m)
{
  stateCallback(true);
  TRACE_F(TRACE_DEBUG2, "TX sending message, type=%d, dataLen=%d, to=%d", m.type, m.dataLen, m.to);

  if (m.dataLen > 0) {
    printBuffer(F("TX payload data:"), m.data, m.dataLen);
  }

  uint8_t data[RH_RF69_MAX_MESSAGE_LEN];
  char *dataChar = (char *)data;
  strcpy(dataChar, RADIO_MSG_PRE);
  data[RADIO_MSG_META_TYPE] = m.type;
  data[RADIO_MSG_META_LEN] = m.dataLen;

  const char *relayBuf = dataChar + RADIO_MSG_META_END;
  memcpy((void *)relayBuf, m.data, m.dataLen);

  int len = RADIO_MSG_META_END + m.dataLen;
  printBuffer(F("TX message data:"), data, len);

  bool ok = p_rd->sendtoWait(data, len, m.to);
  if (ok) {
    TRACE(TRACE_DEBUG2, "TX success");
  }
  else {
    TRACE_F(TRACE_ERROR, "TX failed %d time(s)", ++txFailures);
  }
  stateCallback(false);
  return ok;
}

Message rx(MessageType mt)
{
  Message m;
  while (p_rd->available()) {
    stateCallback(true);
    TRACE(TRACE_DEBUG2, "RX message available");

    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from;

    if (p_rd->recvfromAck(buf, &len, &from)) {
      char *bufChar = (char *)buf;

      TRACE_F(TRACE_DEBUG2, "RX acknowledged, len=%d, from=%d", len, from);
      printBuffer(F("RX message buffer:"), buf, len);

      if (strstr(bufChar, RADIO_MSG_PRE) != nullptr) {
        TRACE(TRACE_DEBUG2, "RX message recognised");

        m.from = from;
        m.type = (MessageType)buf[RADIO_MSG_META_TYPE];
        m.dataLen = buf[RADIO_MSG_META_LEN];
        TRACE_F(TRACE_DEBUG2, "RX decoded, type=%d, dataLen=%d", m.type, m.dataLen);

        if (m.dataLen > 0) {
          uint8_t *payload = buf + RADIO_MSG_META_END;
          char *payloadChar = (char *)payload;
          printBuffer(F("RX payload buffer:"), payload, m.dataLen);

          TRACE(TRACE_DEBUG2, "RX copying data");
          memcpy(m.data, payloadChar, m.dataLen);
        }

        rxCallback(m);
        if (m.type == mt) {
          TRACE(TRACE_DEBUG2, "RX message found");
          break;
        }
      }
      else {
        TRACE(TRACE_ERROR, "RX unexpected message");
      }
    }
    else {
      TRACE(TRACE_ERROR, "RX could not acknowledge");
    }
    stateCallback(false);
  }
  return m;
}

void printBuffer(const __FlashStringHelper *prompt, uint8_t *data, uint8_t dataLen)
{
  char printBuf[PRINT_BUFFER_LEN];
  sprintf(printBuf, "%s ", String(prompt).c_str());
  int printLen = strlen(printBuf);

  for (uint8_t i = 0; i < dataLen; i++) {
    sprintf(printBuf + printLen, "%02Xh", (unsigned int)data[i]);
    printLen += 2;

    if (i != dataLen - 1) {
      printBuf[printLen++] = ' ';
    }
  }
  printBuf[printLen] = '\0';
  TRACE_C(TRACE_DEBUG2, printBuf);
}

} // namespace common
} // namespace radio
} // namespace greenhouse
