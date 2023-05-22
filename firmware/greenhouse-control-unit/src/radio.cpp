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
#define RESET_NODE_DELAY 5000
#define SOIL_TEMP_WEIRD_LIMIT 10
#define SOIL_TEMP_WEIRD_MIN 5
#define SOIL_TEMP_STALE_TIME 600000 // 10 mins
#define KEEP_ALIVE_WAIT_DELAY 1000
#define KEEP_ALIVE_WAIT_TIMEOUT 10000
#define RESET_RELAY_ATTEMPTS 10

RH_RF69 rf69(PIN_CS, PIN_IRQ);
RHReliableDatagram rd(rf69, RHRD_ADDR_SERVER);

namespace greenhouse {
namespace radio {

float soilTemps = k_unknown;
embedded::System *p_system;
int weirdSoilTemps = 0;
unsigned long lastSoilTemp = 0;
bool relayAlive = false;

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
    TRACE(TRACE_ERROR, "RD init failed");
    halt();
  }

  if (!rf69.setFrequency(868.0)) {
    TRACE(TRACE_ERROR, "RF69 set frequency failed");
    halt();
  }

  rf69.setTxPower(20, true);
}

void update() { common::rx(); }

#define KEEP_ALIVE_ATTEMPTS 1

bool keepAliveRelay()
{
  TRACE(TRACE_DEBUG1, "Relay keep alive");

  time_t start = millis();
  relayAlive = false;

  for (int i = 0; i < KEEP_ALIVE_ATTEMPTS; i++) {
    Message txm;
    txm.type = k_keepAlive;
    txm.to = RHRD_ADDR_RELAY;
    common::tx(txm);

    while (!relayAlive) {
      delay(KEEP_ALIVE_WAIT_DELAY);

      common::rx();

      if (!relayAlive && ((millis() - start) > KEEP_ALIVE_WAIT_TIMEOUT)) {
        TRACE(TRACE_ERROR, "Keep alive timeout");
        break;
      }
    }

    if (relayAlive) {
      break;
    }
  }
  return relayAlive;
}

bool checkRelayAlive()
{
  if (!relayAlive) {
    TRACE(TRACE_ERROR, "Relay is dead, keep alive failed");
    return false;
  }
  return true;
}

void status(bool busy)
{
  TRACE_F(TRACE_DEBUG2, "Radio %s", busy ? FCSTR("busy") : FCSTR("free"));
  digitalWrite(LED_BUILTIN, busy ? HIGH : LOW);
}

void rx(Message m)
{
  TRACE_F(TRACE_DEBUG2, "RX handling message, type:%d, dataLen: %d", m.type, m.dataLen);
  switch (m.type) {
  case k_keepAlive: {
    TRACE(TRACE_DEBUG1, "RX keep alive response");
    relayAlive = true;
    break;
  }

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
    TRACE(TRACE_WARN, "RX unknown message type");
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

  // HACK: if soil temps seem too low. this is probably a bug in the relay.
  if (f.n < SOIL_TEMP_WEIRD_MIN) {
    TRACE(TRACE_WARN, "RX ignoring weird soil temp");
    weirdSoilTemps++;

    if ((soilTemps != k_unknown) && (millis() > lastSoilTemp + SOIL_TEMP_STALE_TIME)) {
      TRACE(TRACE_WARN, "RX invalidating soil temp");
      soilTemps = k_unknown;
    }

    if (weirdSoilTemps > SOIL_TEMP_WEIRD_LIMIT) {
      p_system->ResetNodes();
    }
    weirdSoilTemps = 0;
    return;
  }

  lastSoilTemp = millis();

  soilTemps = f.n;
}

void getSoilTempsAsync()
{
  TRACE(TRACE_DEBUG1, "Getting soil temps (async)");

  if (!checkRelayAlive()) {
    return;
  }

  Message txm;
  txm.type = k_soilTemps;
  txm.to = RHRD_ADDR_RELAY;
  common::tx(txm);
}

float getSoilTempsResult() { return soilTemps; }

void windowActuatorRunAll(MotorDirection direction, int runtime)
{
  TRACE(TRACE_DEBUG1, "Window actuator run all");

  if (!checkRelayAlive()) {
    return;
  }

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

  if (!checkRelayAlive()) {
    return;
  }

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

  if (!checkRelayAlive()) {
    return;
  }

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