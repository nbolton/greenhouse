#if RADIO_EN

#include "radio.h"

#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <common.h>
#include <radio_common.h>
#include <trace.h>

#include "embedded/System.h"

#define PIN_IRQ 26
#define PIN_CS 33

#define RHRD_RETRIES 5        // default is 3
#define RHRD_TIMEOUT 300      // default is 200 ms
#define KEEP_ALIVE_FREQ 30000 // 30s

#define SOIL_TEMP_WEIRD_LIMIT 10
#define SOIL_TEMP_VALID_MIN 5
#define SOIL_TEMP_STALE_TIME 600000 // 10 mins

RH_RF69 rf69(PIN_CS, PIN_IRQ);
RHReliableDatagram rd(rf69, RHRD_ADDR_SERVER);

namespace greenhouse {
namespace radio {

float soilTemps = k_unknown;
embedded::System *p_system;
int invalidSoilTemps = 0;
unsigned long lastSoilTemp = 0;
bool relayAlive = false;
time_t lastKeepAlive = 0;

void rx(Message &m);
void status(bool busy);
void rxSoilTemps(Message &m);

void init(embedded::System *system)
{
  p_system = system;

  common::init(&rd, &rf69, rx, status);

  rd.setRetries(RHRD_RETRIES);
  rd.setTimeout(RHRD_TIMEOUT);

  if (!rd.init()) {
    TRACE(TRACE_ERROR, "RD init failed");
    halt();
  }

  if (!rf69.setFrequency(868.0)) {
    TRACE(TRACE_ERROR, "RF69 set frequency failed");
    halt();
  }

  rf69.setTxPower(20, true);
}

void update()
{
  common::rx();

  if ((lastKeepAlive == 0) || (millis() > (lastKeepAlive + KEEP_ALIVE_FREQ))) {
    txKeepAliveRelay();
    lastKeepAlive = millis();
  }
}

bool isRelayAlive() { return relayAlive; }

bool txKeepAliveRelay()
{
  TRACE(TRACE_DEBUG2, "TX relay keep alive");
  Message txm;
  txm.type = MessageType::k_keepAlive;
  txm.to = RHRD_ADDR_RELAY;
  if (common::tx(txm)) {
    relayAlive = true;
  }

  return relayAlive;
}

bool checkRelayAlive()
{
  if (!relayAlive) {
    TRACE(TRACE_ERROR, "Relay is offline");
    return false;
  }
  return true;
}

void status(bool busy)
{
  TRACE_F(TRACE_DEBUG2, "Radio %s", busy ? FCSTR("busy") : FCSTR("free"));
  digitalWrite(LED_BUILTIN, busy ? HIGH : LOW);
}

String pumpStatusToString(radio::PumpStatus ps)
{
  switch (ps) {
  case radio::PumpStatus::k_waitingZ80: {
    return "Waiting for Z80";
  }
  case radio::PumpStatus::k_errorAckRF95: {
    return "Error: No ack from RF95";
  }
  case radio::PumpStatus::k_errorRxFailRF95: {
    return "Error: RF95 rx failure";
  }
  case radio::PumpStatus::k_errorNoReplyRF95: {
    return "Error: No reply from RF95";
  }
  case radio::PumpStatus::k_waitingRF95: {
    return "Waiting for RF95";
  }
  default: {
    return String("Status code: ") + (int)ps;
  }
  }
}

void rx(Message &m)
{
  TRACE_F(TRACE_DEBUG2, "RX handling message, type:%d, dataLen: %d", m.type, m.dataLen);
  switch (m.type) {
  case MessageType::k_keepAlive: {
    TRACE(TRACE_DEBUG1, "RX keep alive response");
    break;
  }

  case MessageType::k_status: {
    int queueSize = (int)m.data[0];
    int sendDrops = (int)m.data[1];
    int queueFails = (int)m.data[2];
    TRACE_F(
      TRACE_DEBUG1,
      "RX relay status: queueSize=%d, sendDrops=%d, queueFails=%d",
      queueSize,
      sendDrops,
      queueFails);
    break;
  }

  case MessageType::k_soilTemps: {
    rxSoilTemps(m);
    break;
  }

  case MessageType::k_pumpSwitch: {
    bool on = m.data[0];
    TRACE_F(TRACE_DEBUG1, "RX pump switch: %s", on ? FCSTR("on") : FCSTR("off"));
    p_system->ReportPumpSwitch(on);
    break;
  }

  case MessageType::k_pumpStatus: {
    radio::PumpStatus ps = (radio::PumpStatus)m.data[0];
    String psString = pumpStatusToString(ps);
    TRACE_F(TRACE_DEBUG1, "RX pump status: %s", psString.c_str());
    p_system->ReportPumpStatus(psString);
    break;
  }

  default: {
    TRACE(TRACE_WARN, "RX unknown message type");
    break;
  }
  }
}

void rxSoilTemps(Message &m)
{
  TRACE(TRACE_DEBUG1, "RX soil temps");

  if (m.type != MessageType::k_soilTemps) {
    TRACE_F(TRACE_DEBUG1, "RX unexpected message (type: %d); expected soil temps", m.type);
    return;
  }

  if (m.dataLen != FLOAT_BYTE_LEN) {
    TRACE_F(TRACE_DEBUG1, "RX invalid float data len: %d; expected %d", m.dataLen, FLOAT_BYTE_LEN);
    return;
  }

  union {
    float n;
    uint8_t data[FLOAT_BYTE_LEN];
  } f;

  memcpy(f.data, m.data, FLOAT_BYTE_LEN);
  TRACE_F(TRACE_DEBUG1, "RX soil temps: %.2f", f.n);

  if (f.n < SOIL_TEMP_VALID_MIN) {
    // HACK: if soil temps seem too low. this is probably a bug in the relay.
    TRACE(TRACE_WARN, "RX ignoring invalid soil temp");
    invalidSoilTemps++;

    if ((soilTemps != k_unknown) && (millis() > lastSoilTemp + SOIL_TEMP_STALE_TIME)) {
      TRACE(TRACE_WARN, "RX invalidating soil temp");
      soilTemps = k_unknown;
    }

    if (invalidSoilTemps > SOIL_TEMP_WEIRD_LIMIT) {
      p_system->ResetNodes();
    }
    invalidSoilTemps = 0;
    return;
  }

  lastSoilTemp = millis();

  soilTemps = f.n;
}

void txGetRelayStatusAsync()
{
  TRACE(TRACE_DEBUG1, "TX get relay status (async)");

  if (!checkRelayAlive()) {
    return;
  }

  Message txm;
  txm.type = MessageType::k_status;
  txm.to = RHRD_ADDR_RELAY;
  common::tx(txm);
}

void txGetSoilTempsAsync()
{
  TRACE(TRACE_DEBUG1, "TX get soil temps (async)");

  if (!checkRelayAlive()) {
    return;
  }

  Message txm;
  txm.type = MessageType::k_soilTemps;
  txm.to = RHRD_ADDR_RELAY;
  common::tx(txm);
}

float getSoilTempsResult() { return soilTemps; }

void txWindowActuatorRunAll(MotorDirection direction, int runtime)
{
  TRACE(TRACE_DEBUG1, "TX window actuator run all");

  if (!checkRelayAlive()) {
    return;
  }

  Message m;
  m.to = RHRD_ADDR_RELAY;
  m.type = MessageType::k_windowActuatorRunAll;
  m.data[RADIO_NODE_WA_D] = direction;
  m.data[RADIO_NODE_WA_R] = runtime;
  m.dataLen = 2;
  common::tx(m);
}

void txWindowActuatorSetup(int leftSpeed, int rightSpeed)
{
  TRACE(TRACE_DEBUG1, "TX window actuator setup");

  if (!checkRelayAlive()) {
    return;
  }

  Message m;
  m.to = RHRD_ADDR_RELAY;
  m.type = MessageType::k_windowActuatorSetup;
  m.data[RADIO_NODE_WA_L] = leftSpeed;
  m.data[RADIO_NODE_WA_R] = rightSpeed;
  m.dataLen = 2;
  common::tx(m);
}

void txPumpSwitch(bool on)
{
  TRACE_F(TRACE_DEBUG1, "TX switch pump %s", on ? FCSTR("on") : FCSTR("off"));

  if (!checkRelayAlive()) {
    return;
  }

  Message m;
  m.to = RHRD_ADDR_RELAY;
  m.type = MessageType::k_pumpSwitch;
  m.data[0] = on;
  m.dataLen = 1;
  common::tx(m);
}

} // namespace radio
} // namespace greenhouse

#endif // RADIO_EN