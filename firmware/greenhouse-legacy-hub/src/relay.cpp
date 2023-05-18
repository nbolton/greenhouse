#include "relay.h"

#include <Arduino.h>
#include <RHReliableDatagram.h>
#include <RH_RF69.h>
#include <radio_common.h>

#include "common.h"
#include "legacy/NodeRadio.h"
#include "legacy/PumpRadio.h"

#define PIN_IRQ PA2
#define PIN_CS PA1

namespace gr = greenhouse::radio;

using namespace legacy;
using namespace gr;

RH_RF69 rf69(PIN_CS, PIN_IRQ);
RHReliableDatagram rd(rf69, RHRD_ADDR_RELAY);
NodeRadio *p_nodeRadio;
PumpRadio *p_pumpRadio;

namespace relay {

void rx(Message m);
void state(bool busy) { led(busy ? LOW : HIGH); }
void relaySoilTemps();
void relayWindowActuatorRunAll(Message m);

void init(NodeRadio &nr, PumpRadio &pr) {
  p_nodeRadio = &nr;
  p_pumpRadio = &pr;

  common::init(&rd, &rf69, rx, state);

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

void rx(Message m) {
  switch (m.type) {
    case k_reset: {
      NVIC_SystemReset();
      break;
    }
    case k_soilTemps: {
      relaySoilTemps();
      break;
    }
    case k_windowActuatorRunAll: {
      relayWindowActuatorRunAll(m);
      break;
    }
    case k_windowActuatorSetup: {
      int l = (int)m.data[RADIO_NODE_WA_L];
      int r = (int)m.data[RADIO_NODE_WA_R];
      p_nodeRadio->SetWindowSpeeds(l, r);
      break;
    }
    case k_pumpSwitch: {
      p_pumpRadio->SwitchPump((bool)m.data[0]);
      break;
    }
  }
}

void relaySoilTemps() {
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

  common::tx(m);
}

void relayWindowActuatorRunAll(Message m) {
  gr::MotorDirection d = (gr::MotorDirection)m.data[RADIO_NODE_WA_D];
  byte r = (byte)m.data[RADIO_NODE_WA_R];

  legacy::MotorDirection ld;
  if (d == gr::MotorDirection::k_windowExtend) {
    ld = legacy::MotorDirection::k_windowExtend;
  } else if (d == gr::MotorDirection::k_windowRetract) {
    ld = legacy::MotorDirection::k_windowRetract;
  }

  p_nodeRadio->MotorRunAll(ld, r);
}

void txPumpStatus(const char *status) {
  Message m;
  m.to = RHRD_ADDR_SERVER;
  m.type = MessageType::k_pumpStatus;
  strcpy((char *)m.data, status);
  m.dataLen = strlen(status) + 1;  // add null char
  common::tx(m);
}

void txPumpSwitch(bool on) {
  Message m;
  m.to = RHRD_ADDR_SERVER;
  m.type = MessageType::k_pumpSwitch;
  m.data[0] = (unsigned char)on;
  m.dataLen = 1;
  common::tx(m);
}

}  // namespace relay
