#pragma once

#include <Arduino.h>

#define RHRD_RETRIES 10  // default is 3
#define RHRD_TIMEOUT 500 // default is 200 ms

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
#define RADIO_MSG_DATA_SIZE 100

#define RADIO_NODE_WA_D 0
#define RADIO_NODE_WA_R 1
#define RADIO_NODE_WA_L 0
#define RADIO_NODE_WA_R 1

#define FLOAT_BYTE_LEN 4

class RHReliableDatagram;
class RH_RF69;

namespace greenhouse {
namespace radio {

enum MotorDirection { k_windowExtend, k_windowRetract };

enum MessageType { //
  k_messageTypeNotSet,

  k_soilTemps,
  k_windowActuatorRunAll,
  k_windowActuatorSetup,
  k_pumpSwitch,
  k_pumpStatus,

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

namespace common {

typedef void (*RxCallback)(Message m);
typedef void (*StateCallback)(bool busy);

void init(
  ::RHReliableDatagram *rd, ::RH_RF69 *rf69, RxCallback _rxCallback, StateCallback _stateCallback);
Message rx(MessageType mt = k_messageTypeNotSet);
bool tx(Message m);
void printBuffer(const __FlashStringHelper *prompt, uint8_t *data, uint8_t dataLen);

} // namespace common

} // namespace radio
} // namespace greenhouse
