#pragma once

#define RHRD_ADDR_SERVER 0
#define RHRD_ADDR_NODE1 1
#define RHRD_ADDR_NODE2 2
#define RHRD_ADDR_NODE3 3
#define RHRD_ADDR_RELAY 10

#define RADIO_MSG_PRE "garden"
#define RADIO_MSG_PROTO RADIO_MSG_PRE ":%c,%c:"
#define RADIO_MSG_SEP_CHAR ':'
#define RADIO_MSG_SEP_STR ":"
#define RADIO_MSG_DATA_SIZE 100

namespace greenhouse {
namespace radio {

enum MessageType { //
  k_messageTypeNotSet,
  k_messageAck,

  k_requestTemps,
  k_windowActuatorRunAll,
  k_windowActuatorSetup,
  k_switchPump
};

struct Message {
  MessageType type = k_messageTypeNotSet;
  unsigned char data[RADIO_MSG_DATA_SIZE];
  unsigned char dataLen = 0;
  unsigned char attempts = 0;
  unsigned char id = 0;
};

} // namespace radio
} // namespace greenhouse
