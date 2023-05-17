#if RADIO_EN

#include "Radio.h"

#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <common.h>
#include <list>
#include <log.h>
#include <radio_common.h>

#define PIN_IRQ 26
#define PIN_CS 33
#define TEMP_REQ_FREQ 60000
#define RHRD_RETRIES 10  // default is 3
#define RHRD_TIMEOUT 100 // default is 200 ms

RH_RF69 rf69(PIN_CS, PIN_IRQ);
RHReliableDatagram rd(rf69, RHRD_ADDR_SERVER);

namespace greenhouse {
namespace radio {

float soilTemp;
unsigned long lastTempReq;

void rx();
void rx(MessageType type, int dataLen, char *data);
void tx();
void tx(Message m);
void rxPump(const char *data, int length);
void rxNode(const char *data, int length);
void requestSoilTemp();

void init()
{
  rd.setRetries(RHRD_RETRIES);
  rd.setTimeout(RHRD_TIMEOUT);

  if (!rd.init()) {
    halt();
  }

  if (!rf69.setFrequency(868.0)) {
    halt();
  }

  rf69.setTxPower(20, true);
}

void update()
{
  rx();

  if (millis() > lastTempReq + TEMP_REQ_FREQ) {
    TRACE("Requesting soil temp");
    requestSoilTemp();
    lastTempReq = millis();
  }
}

void tx(Message m)
{
  char dataChar[RH_RF69_MAX_MESSAGE_LEN];
  uint8_t *data = (uint8_t *)&dataChar;

  sprintf(dataChar, RADIO_MSG_PROTO, m.type, m.dataLen);
  int preLen = strlen(dataChar);

  const char *relayBuf = dataChar + preLen;
  memcpy((void *)relayBuf, m.data, m.dataLen);

  rd.sendtoWait(data, RH_RF69_MAX_MESSAGE_LEN, RHRD_ADDR_SERVER);
}

void rx()
{
  if (rd.available()) {
    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    uint8_t from;

    if (rd.recvfromAck(buf, &len, &from)) {

      char *bufChar = (char *)buf;
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

void rx(MessageType type, int dataLen, char *data) {}

float soilTempResult() { return soilTemp; }

void requestSoilTemp()
{
  Message m;
  m.type = k_requestTemps;
  tx(m);
}

void windowActuatorRunAll(MotorDirection direction, int runtime)
{
  Message m;
  m.type = k_windowActuatorRunAll;
  m.data[0] = direction;
  m.data[1] = runtime;
  m.dataLen = 2;
  tx(m);
}

void windowActuatorSetup(int leftSpeed, int rightSpeed)
{
  Message m;
  m.type = k_windowActuatorSetup;
  m.data[0] = leftSpeed;
  m.data[1] = rightSpeed;
  m.dataLen = 2;
  tx(m);
}

void switchPump(bool on)
{
  Message m;
  m.type = k_switchPump;
  m.data[0] = on;
  m.dataLen = 1;
  tx(m);
}

} // namespace radio
} // namespace greenhouse

#endif // RADIO_EN