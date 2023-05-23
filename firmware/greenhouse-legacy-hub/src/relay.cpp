#include "relay.h"

#include <Arduino.h>
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <list>
#include <radio_common.h>

#include "common.h"
#include "legacy/NodeRadio.h"
#include "legacy/PumpRadio.h"

#define PIN_CS PA1
#define PIN_IRQ PA2

#define QUEUE_MAX 20 // consider message data buffer size
#define QUEUE_DELAY 100
#define QUEUE_RETRY_FREQ 1000
#define QUEUE_DEFAULT_TTL 30000
#define WINDOW_ACTUATOR_RUN_TTL 60000

#define RHRD_RETRIES 10  // default is 3
#define RHRD_TIMEOUT 200 // default is 200 ms

namespace gr = greenhouse::radio;

using namespace legacy;
using namespace gr;

struct RelayMessage {
  Message original;
  int attempts = 0;
  time_t nextAttempt = 0;
  int ttl = 0;
  time_t queuedAt = 0;
  bool ok = false;
  bool dropped = false;
};

typedef std::list<RelayMessage> RelayMessageList;

RH_RF69 rf69(PIN_CS, PIN_IRQ);
RHReliableDatagram rd(rf69, RHRD_ADDR_RELAY);
NodeRadio *p_nodeRadio;
PumpRadio *p_pumpRadio;
RelayMessageList messages;
RelayMessage *p_rmLast;
int droppedMessages = 0;
int queueFails = 0;
int queueSize = 0;

namespace relay {

void queueMessage(Message &m);
bool process(Message &m);
void state(bool busy) { led(busy ? LOW : HIGH); }
bool relaySoilTemps();
bool relayWindowActuatorRunAll(Message &m);
bool txKeepAlive();
bool txStatus();

void init(NodeRadio &nr, PumpRadio &pr)
{
  p_nodeRadio = &nr;
  p_pumpRadio = &pr;

  common::init(&rd, &rf69, queueMessage, state);

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
  queueSize = messages.size();
  if (messages.size() > 0) {
    RelayMessageList::iterator it;
    for (it = messages.begin(); it != messages.end(); ++it) {

      common::rx();

      RelayMessage &rm = *it;
      p_rmLast = &rm;

      if (millis() < rm.nextAttempt) {
        continue;
      }

      rm.attempts++;
      rm.ok = process(rm.original);

      if (rm.ok || (millis() > (rm.queuedAt + rm.ttl))) {
        if (!rm.ok) {
          rm.dropped = true;
          droppedMessages++;
        }
        messages.erase(it);
        break;
      }
      else {
        rm.nextAttempt = millis() + QUEUE_RETRY_FREQ;
      }
    }
  }
  else {
    common::rx();
  }
}

bool process(Message &m)
{
  switch (m.type) {
  case k_keepAlive: {
    return txKeepAlive();
  }
  case k_status: {
    return txStatus();
  }
  case k_reset: {
    NVIC_SystemReset();
    return true; // pointless?
  }
  case k_soilTemps: {
    return relaySoilTemps();
  }
  case k_windowActuatorRunAll: {
    return relayWindowActuatorRunAll(m);
  }
  case k_windowActuatorSetup: {
    int l = (int)m.data[RADIO_NODE_WA_L];
    int r = (int)m.data[RADIO_NODE_WA_R];
    return p_nodeRadio->SetWindowSpeeds(l, r);
  }
  case k_pumpSwitch: {
    return p_pumpRadio->SwitchPump((bool)m.data[0]);
  }
  }
  return false;
}

bool txStatus()
{
  Message m;
  m.to = RHRD_ADDR_SERVER;
  m.type = MessageType::k_status;

  m.data[0] = messages.size();
  m.data[1] = droppedMessages;
  m.data[2] = queueFails;
  m.dataLen = 3;

  return common::tx(m);
}

bool txKeepAlive()
{
  Message m;
  m.to = RHRD_ADDR_SERVER;
  m.type = MessageType::k_keepAlive;
  return common::tx(m);
}

int getTimeToLive(MessageType t)
{
  switch (t) {
  case MessageType::k_windowActuatorRunAll: {
    return WINDOW_ACTUATOR_RUN_TTL;
  }
  }
  return QUEUE_DEFAULT_TTL;
}

void queueMessage(Message &m)
{
  if (messages.size() > QUEUE_MAX) {
    queueFails++;
    return;
  }

  RelayMessage rm;
  rm.original = m;
  rm.queuedAt = millis();
  rm.ttl = getTimeToLive(m.type);
  messages.push_back(rm);
}

bool relaySoilTemps()
{
  float temps = p_nodeRadio->GetSoilTemps();

  union {
    float n;
    uint8_t data[FLOAT_BYTE_LEN];
  } f;

  f.n = temps;

  Message m;
  m.to = RHRD_ADDR_SERVER;
  m.type = MessageType::k_soilTemps;

  memcpy(m.data, f.data, FLOAT_BYTE_LEN);
  m.dataLen = FLOAT_BYTE_LEN;

  return common::tx(m);
}

bool relayWindowActuatorRunAll(Message &m)
{
  gr::MotorDirection d = (gr::MotorDirection)m.data[RADIO_NODE_WA_D];
  byte r = (byte)m.data[RADIO_NODE_WA_R];

  legacy::MotorDirection ld;
  if (d == gr::MotorDirection::k_windowExtend) {
    ld = legacy::MotorDirection::k_windowExtend;
  }
  else if (d == gr::MotorDirection::k_windowRetract) {
    ld = legacy::MotorDirection::k_windowRetract;
  }

  return p_nodeRadio->MotorRunAll(ld, r);
}

bool txPumpStatus(PumpStatus status)
{
  Message m;
  m.to = RHRD_ADDR_SERVER;
  m.type = MessageType::k_pumpStatus;
  m.data[0] = status;
  m.dataLen = 1;
  return common::tx(m);
}

bool txPumpSwitch(bool on)
{
  Message m;
  m.to = RHRD_ADDR_SERVER;
  m.type = MessageType::k_pumpSwitch;
  m.data[0] = (unsigned char)on;
  m.dataLen = 1;
  return common::tx(m);
}

} // namespace relay
