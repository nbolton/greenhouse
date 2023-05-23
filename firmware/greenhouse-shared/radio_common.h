#pragma once

#include <Arduino.h>

#define RHRD_ADDR_SERVER 0
#define RHRD_ADDR_NODE1 1
#define RHRD_ADDR_NODE2 2
#define RHRD_ADDR_NODE3 3
#define RHRD_ADDR_RELAY 10

#define RADIO_MSG_PRE "garden:"
#define RADIO_MSG_PRE_END strlen(RADIO_MSG_PRE)
#define RADIO_MSG_META_TYPE RADIO_MSG_PRE_END
#define RADIO_MSG_META_LEN RADIO_MSG_PRE_END + 1
#define RADIO_MSG_META_END RADIO_MSG_PRE_END + 2
#define RADIO_MSG_DATA_SIZE 10

#define RADIO_NODE_WAR_N 0
#define RADIO_NODE_WAR_D 1
#define RADIO_NODE_WAR_R 2
#define RADIO_NODE_WAS_N 0
#define RADIO_NODE_WAS_S 1

#define RADIO_RELAY_STATUS_QUEUE 0
#define RADIO_RELAY_STATUS_DROP 1
#define RADIO_RELAY_STATUS_FAIL 2
#define RADIO_RELAY_STATUS_NSL 3
#define RADIO_RELAY_STATUS_NSR 4
#define RADIO_RELAY_STATUS_NEL 5
#define RADIO_RELAY_STATUS_NER 6

#define FLOAT_BYTE_LEN 4

class RHReliableDatagram;
class RH_RF69;

namespace greenhouse {
namespace radio {

enum MessageType { //
  k_messageTypeNotSet,

  k_soilTemps,
  k_windowActuatorRun,
  k_windowActuatorSetup,
  k_pumpSwitch,
  k_pumpStatus,

  k_status = 253,
  k_keepAlive = 254,
  k_reset = 255
};

struct Message {
  uint8_t to;
  uint8_t from;
  MessageType type = k_messageTypeNotSet;
  unsigned char data[RADIO_MSG_DATA_SIZE];
  unsigned char dataLen = 0;
};

enum MotorDirection { k_windowExtend, k_windowRetract };

enum Window { k_leftWindow, k_rightWindow };

enum PumpStatus {
  k_waitingZ80,
  k_errorAckRF95,
  k_errorRxFailRF95,
  k_errorNoReplyRF95,
  k_waitingRF95
};

namespace common {

typedef void (*RxCallback)(Message &m);
typedef void (*StateCallback)(bool busy);

void init(
  ::RHReliableDatagram *rd, ::RH_RF69 *rf69, RxCallback _rxCallback, StateCallback _stateCallback);
void rx();
bool tx(Message &m);
void printBuffer(const __FlashStringHelper *prompt, uint8_t *data, uint8_t dataLen);

} // namespace common

} // namespace radio
} // namespace greenhouse
