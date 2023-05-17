#if PUMP_RADIO_EN

#include "PumpRadio.h"

#include "common.h"

#include <Arduino.h>
#include <RH_RF95.h>
#include <SPI.h>

#include <string>

#define RFM95_CS 5
#define RFM95_IRQ 4

RH_RF95 rf95(RFM95_CS, RFM95_IRQ);

bool pumpMessagePending = false;
bool pumpMessageAckWait = false;
bool pumpSwitchOn = false;
unsigned long pumpMessageAckWaitStart = 0;
uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);

#define LORA_ACK_TIMEOUT 3000
#define PUMP_ACK_TIMEOUT 10000

namespace legacy {

PumpRadio::PumpRadio() {}

void PumpRadio::Init()
{
  TRACE("Init RF95 LoRa");
  if (!s_rf95.init()) {
    TRACE("RF95 LoRa init failed");
    while (true) {
      delay(1);
    }
  }
}

void PumpRadio::Update()
{
  if (s_rf95.available()) {
    if (s_rf95.recv(buf, &len)) {
      const char *bufChar = (char *)buf;
      TRACE_F("LoRa RX: %s", +bufChar);

      if (std::string(bufChar).find("z80") != std::string::npos) {
        TRACE("Message received from Z80");

        if (std::string(bufChar).find("pump=1") != std::string::npos) {
          LowerPumpStatus("Pump running");
          LowerPumpOn(true);
          pumpMessageAckWait = false;
        }
        else {
          LowerPumpStatus("Pump stopped");
          LowerPumpOn(false);
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
  s_rf95.send(data, sizeof(data));

  s_rf95.waitPacketSent();
  if (s_rf95.waitAvailableTimeout(LORA_ACK_TIMEOUT)) {
    if (s_rf95.recv(buf, &len)) {
      const char *bufChar = (char *)buf;
      TRACE_F("LoRa RX: %s", bufChar);

      if (std::string(bufChar).find("ack") != std::string::npos) {
        TRACE("LoRa ack OK");
        LowerPumpStatus("Waiting for Z80");

        pumpMessagePending = false;
        pumpMessageAckWait = true;
        pumpMessageAckWaitStart = millis();
      }
      else {
        TRACE("LoRa error: No ack");
        LowerPumpStatus("LoRa error: Ack failed");
      }
    }
    else {
      TRACE("LoRa RX failed");
      LowerPumpStatus("LoRa error: RX failed");
    }
  }
  else {
    TRACE_F("LoRa error: No reply within %dms", LORA_ACK_TIMEOUT);
    LowerPumpStatus("LoRa error: No reply");
  }
}

void PumpRadio::SwitchPump(bool on)
{
  pumpMessagePending = true;
  pumpSwitchOn = on;
  LowerPumpStatus("Waiting for LoRa");
}

} // namespace legacy

#endif // PUMP_RADIO_EN