#if LORA_EN

#include "PumpRadio.h"

#include <Arduino.h>
#include <RH_RF95.h>
#include <SPI.h>

#include <string>

#include "../../common/common.h"
#include "../../common/log.h"

#define RFM95_CS 5
#define RFM95_IRQ 4

RH_RF95 rf95(RFM95_CS, RFM95_IRQ);

bool pumpMessagePending = false;
bool pumpMessageAckWait = false;
bool pumpSwitchOn = false;
unsigned long pumpMessageAckWaitStart = 0;

#define LORA_ACK_TIMEOUT 3000
#define PUMP_ACK_TIMEOUT 10000

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);

namespace embedded {
namespace greenhouse {

PumpRadio::PumpRadio() : m_system(nullptr) {}

void PumpRadio::Init(embedded::greenhouse::ISystem *system)
{
  m_system = system;
  TRACE("Init RF95 LoRa");
  if (!rf95.init()) {
    TRACE("RF95 LoRa init failed");
    while (true) {
      delay(1);
    }
  }
}

void PumpRadio::Update()
{
  if (rf95.available()) {
    if (rf95.recv(buf, &len)) {
      const char *bufChar = (char *)buf;
      TRACE_F("LoRa RX: %s", +bufChar);

      if (std::string(bufChar).find("z80") != std::string::npos) {
        TRACE("Message received from Z80");

        if (std::string(bufChar).find("pump=1") != std::string::npos) {
          m_system->LowerPumpStatus("Pump running");
          m_system->LowerPumpOn(true);
          pumpMessageAckWait = false;
        }
        else {
          m_system->LowerPumpStatus("Pump stopped");
          m_system->LowerPumpOn(false);
          pumpMessageAckWait = false;
        }

        if (std::string(bufChar).find("online") != std::string::npos) {
          TRACE("Z80 is online, sending pump message");
          pumpMessagePending = true;
        }
      }
    }
    else {
      TRACE("LoRa RX failed");
    }
  }

  if (pumpMessageAckWait) {
    if (millis() > (pumpMessageAckWaitStart + PUMP_ACK_TIMEOUT)) {
      TRACE("Pump message timeout, retrying");
      pumpMessagePending = true;
    }
  }

  if (pumpMessagePending) {
    SendPumpMessage();
  }

  delay(10);
}

void PumpRadio::SendPumpMessage()
{
  uint8_t data[11];
  strcpy((char *)data, pumpSwitchOn ? "z80:pump=1" : "z80:pump=0");

  TRACE_F("LoRa TX: %s", (char *)data);
  rf95.send(data, sizeof(data));

  rf95.waitPacketSent();
  if (rf95.waitAvailableTimeout(LORA_ACK_TIMEOUT)) {
    if (rf95.recv(buf, &len)) {
      const char *bufChar = (char *)buf;
      TRACE_F("LoRa RX: %s", bufChar);

      if (std::string(bufChar).find("ack") != std::string::npos) {
        TRACE("LoRa ack OK");
        m_system->LowerPumpStatus("Waiting for Z80");

        pumpMessagePending = false;
        pumpMessageAckWait = true;
        pumpMessageAckWaitStart = millis();
      }
      else {
        TRACE("LoRa error: No ack");
        m_system->LowerPumpStatus("LoRa error: Ack failed");
      }
    }
    else {
      TRACE("LoRa RX failed");
      m_system->LowerPumpStatus("LoRa error: RX failed");
    }
  }
  else {
    TRACE_F("LoRa error: No reply within %dms", LORA_ACK_TIMEOUT);
    m_system->LowerPumpStatus("LoRa error: No reply");
  }
}

void PumpRadio::SwitchPump(bool on)
{
  pumpMessagePending = true;
  pumpSwitchOn = on;
  m_system->LowerPumpStatus("Waiting for LoRa");
}

} // namespace greenhouse
} // namespace embedded

#endif // LORA_EN