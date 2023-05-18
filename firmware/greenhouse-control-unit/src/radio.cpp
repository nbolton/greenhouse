#if RADIO_EN

#include "radio.h"

#include "embedded/System.h"

#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <common.h>
#include <list>
#include <radio_common.h>
#include <trace.h>

#define PIN_IRQ 26
#define PIN_CS 33

RH_RF69 rf69(PIN_CS, PIN_IRQ);
RHReliableDatagram rd(rf69, RHRD_ADDR_SERVER);

namespace greenhouse {
namespace radio {

float soilTemps = k_unknown;
embedded::System *p_system;

void rx(Message m);
void status(bool busy);
void rxSoilTemps(Message m);

void init(embedded::System *system)
{
  p_system = system;

  common::init(&rd, &rf69, rx, status);

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

void update() { common::rx(); }

void status(bool busy)
{
  TRACE_F(TRACE_DEBUG2, "Radio %s", busy ? FCSTR("busy") : FCSTR("free"));
  digitalWrite(LED_BUILTIN, busy ? HIGH : LOW);
}

void rx(Message m)
{
  TRACE_F(TRACE_DEBUG1, "RX handling message, type:%d, dataLen: %d", m.type, m.dataLen);
  switch (m.type) {

  case k_soilTemps: {
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
    TRACE_F(TRACE_DEBUG1, "RX pump status: %s", m.data);
    p_system->ReportPumpStatus((const char *)m.data);
    break;
  }

  default: {
    TRACE(TRACE_DEBUG1, "RX unknown message type");
    break;
  }
  }
}

void rxSoilTemps(Message m)
{
  TRACE(TRACE_DEBUG1, "RX soil temps");

  if (m.type != k_soilTemps) {
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
  soilTemps = f.n;
}

void getSoilTempsAsync()
{
  TRACE(TRACE_DEBUG1, "Getting soil temps (async)");

  Message txm;
  txm.type = k_soilTemps;
  txm.to = RHRD_ADDR_RELAY;
  common::tx(txm);
}

float getSoilTempsResult() { return soilTemps; }

void windowActuatorRunAll(MotorDirection direction, int runtime)
{
  TRACE(TRACE_DEBUG1, "Window actuator run all");

  Message m;
  m.to = RHRD_ADDR_RELAY;
  m.type = k_windowActuatorRunAll;
  m.data[RADIO_NODE_WA_D] = direction;
  m.data[RADIO_NODE_WA_R] = runtime;
  m.dataLen = 2;
  common::tx(m);
}

void windowActuatorSetup(int leftSpeed, int rightSpeed)
{
  TRACE(TRACE_DEBUG1, "Window actuator setup");

  Message m;
  m.to = RHRD_ADDR_RELAY;
  m.type = k_windowActuatorSetup;
  m.data[RADIO_NODE_WA_L] = leftSpeed;
  m.data[RADIO_NODE_WA_R] = rightSpeed;
  m.dataLen = 2;
  common::tx(m);
}

void pumpSwitch(bool on)
{
  TRACE_F(TRACE_DEBUG1, "Switch pump %s", on ? FCSTR("on") : FCSTR("off"));

  Message m;
  m.to = RHRD_ADDR_RELAY;
  m.type = k_pumpSwitch;
  m.data[0] = on;
  m.dataLen = 1;
  common::tx(m);
}

} // namespace radio
} // namespace greenhouse

#endif // RADIO_EN