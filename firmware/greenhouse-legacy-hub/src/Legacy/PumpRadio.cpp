#if PUMP_RADIO_EN

#include "PumpRadio.h"

#include "common.h"
#include "relay.h"

#include <Arduino.h>
#include <RH_RF95.h>
#include <SPI.h>

#include <string>

#define RFM95_CS PA3
#define RFM95_IRQ PA4

RH_RF95 s_rf95(RFM95_CS, RFM95_IRQ);

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
  if (!s_rf95.init()) {
    halt();
  }
}

void PumpRadio::Update()
{
  if (s_rf95.available()) {
    led(LOW);
    if (s_rf95.recv(buf, &len)) {
      const char *bufChar = (char *)buf;
      TRACE_F("RF95 RX: %s", +bufChar);

      if (std::string(bufChar).find("z80") != std::string::npos) {
        TRACE("Message received from Z80");

        if (std::string(bufChar).find("pump=1") != std::string::npos) {
          relay::txPumpStatus("Pump running");
          relay::txPumpSwitch(true);
          pumpMessageAckWait = false;
        }
        else {
          relay::txPumpStatus("Pump stopped");
          relay::txPumpSwitch(false);
          pumpMessageAckWait = false;
        }

        if (std::string(bufChar).find("online") != std::string::npos) {
          TRACE("Z80 is online, sending pump message");
          pumpMessagePending = true;
        }
      }
    }
    else {
      TRACE("RF95 RX failed");
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

  led(HIGH);
}

void PumpRadio::SendPumpMessage()
{
  led(LOW);

  uint8_t data[11];
  strcpy((char *)data, pumpSwitchOn ? "z80:pump=1" : "z80:pump=0");

  TRACE_F("RF95 TX: %s", (char *)data);
  s_rf95.send(data, sizeof(data));

  s_rf95.waitPacketSent();
  if (s_rf95.waitAvailableTimeout(LORA_ACK_TIMEOUT)) {
    if (s_rf95.recv(buf, &len)) {
      const char *bufChar = (char *)buf;
      TRACE_F("RF95 RX: %s", bufChar);

      if (std::string(bufChar).find("ack") != std::string::npos) {
        TRACE("RF95 ack OK");
        relay::txPumpStatus("Waiting for Z80");

        pumpMessagePending = false;
        pumpMessageAckWait = true;
        pumpMessageAckWaitStart = millis();
      }
      else {
        TRACE("RF95 error: No ack");
        relay::txPumpStatus("RF95 error: Ack failed");
      }
    }
    else {
      TRACE("RF95 RX failed");
      relay::txPumpStatus("RF95 error: RX failed");
    }
  }
  else {
    TRACE_F("RF95 error: No reply within %dms", LORA_ACK_TIMEOUT);
    relay::txPumpStatus("RF95 error: No reply");
  }

  led(HIGH);
}

void PumpRadio::SwitchPump(bool on)
{
  pumpMessagePending = true;
  pumpSwitchOn = on;
  relay::txPumpStatus("Waiting for RF95");
}

} // namespace legacy

#endif // PUMP_RADIO_EN