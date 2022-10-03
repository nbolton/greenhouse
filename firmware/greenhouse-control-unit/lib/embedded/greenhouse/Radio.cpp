#if RADIO_EN

#include "Radio.h"

#include "../../common/common.h"
#include "ISystem.h"

#include <RH_ASK.h>

#define RADIO_TRACE

#define BIT_RATE 2000
#define RX_PIN D0
#define TX_PIN D4
#define PTT_PIN -1 // disable; using SR instead
#define SR_PIN_EN_TX 15
#define RX_TIMEOUT 2000
#define TX_WAIT_DELAY 20
#define TEMP_OFFSET -1.2
#define TEMP_UNKNOWN 255
#define TX_RETRY_MAX 5

#ifndef RADIO_TRACE
#undef TRACE
#define TRACE(l)
#undef TRACE_F
#define TRACE_F(...)
#endif // RADIO_TRACE

namespace embedded {
namespace greenhouse {

static RH_ASK s_driver(BIT_RATE, RX_PIN, TX_PIN, PTT_PIN);
static uint8_t s_rxBuf[GH_LENGTH];
static uint8_t s_txBuf[GH_LENGTH];
static uint8_t s_sequence = 1; // start at 1; reciever starts at 0

void printBuffer(const __FlashStringHelper *prompt, const uint8_t *buf, uint8_t len);

Radio::Radio() : m_system(nullptr) {}

void Radio::Setup()
{
  if (!s_driver.init()) {
    TRACE("Fatal: Radio driver init failed");
    common::halt();
  }

  for (int i = 0; i < NODES_MAX; i++) {
    m_nodes[i].Radio(*this);
  }

  m_nodes[radio::k_nodeRightWindow].Address(GH_ADDR_NODE_1);
  m_nodes[radio::k_nodeLeftWindow].Address(GH_ADDR_NODE_2);
}

void Radio::sr(int pin, bool set) { m_system->ShiftRegister(pin, set); }

bool Radio::Send(radio::SendDesc &sendDesc)
{
  bool rx = false;
  for (int i = 0; i < TX_RETRY_MAX; i++) {
    
    TRACE_F("Radio sending, attempt: %d of %d", i + 1, TX_RETRY_MAX);

    sr(SR_PIN_EN_TX, true);
    delay(TX_WAIT_DELAY);

    GH_TO(s_txBuf) = sendDesc.to;
    GH_FROM(s_txBuf) = GH_ADDR_MAIN;
    GH_CMD(s_txBuf) = sendDesc.cmd;
    GH_DATA_1(s_txBuf) = sendDesc.data1;
    GH_DATA_2(s_txBuf) = sendDesc.data2;

    s_sequence = s_sequence % 256;
    GH_SEQ(s_txBuf) = s_sequence;

    s_driver.send(s_txBuf, GH_LENGTH);
    s_driver.waitPacketSent();
    printBuffer(F("Radio sent data: "), s_txBuf, GH_LENGTH);

    sr(SR_PIN_EN_TX, false);

    unsigned long start = millis();
    while (millis() < (start + RX_TIMEOUT)) {
      uint8_t s_rxBufLen = sizeof(s_rxBuf);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
      if (s_driver.recv(s_rxBuf, &s_rxBufLen)) {
#pragma GCC diagnostic pop

        printBuffer(F("Radio got data: "), s_rxBuf, GH_LENGTH);

        if (GH_TO(s_rxBuf) == GH_ADDR_MAIN) {
          if (GH_CMD(s_rxBuf) == GH_CMD_ERROR) {
            TRACE_F("Radio error code: %d", GH_DATA_1(s_rxBuf));
          }
          else if (GH_CMD(s_rxBuf) != sendDesc.expectCmd) {
            TRACE("Radio error, unexpected command");
          }
          else if (sendDesc.okCallback != NULL) {
            sendDesc.okCallbackReturn = sendDesc.okCallback(sendDesc.okCallbackArg);
          }

          rx = true;
          break;
        }
        else {
          TRACE("Radio ignoring message (address mismatch)");
          // don't report an error, this is fine
        }
      }
      delay(1); // prevent WDT reset
    }

    if (rx) {
      break;
    }
    else {
      TRACE("Radio error, timeout");
    }
  }

  return rx;
}

// begin node class

namespace radio {

bool tempDevsOk(void *arg)
{
  radio::TempData *data = (radio::TempData *)arg;
  data->devs = GH_DATA_1(s_rxBuf);
  TRACE_F("Got temperature device count: ", data->devs);
  return true;
}

bool tempDataOk(void* argV) {
  TempDataCallbackArg* arg = (TempDataCallbackArg*)argV;
  const uint8_t a = GH_DATA_1(s_rxBuf);
  const uint8_t b = GH_DATA_2(s_rxBuf);
  TRACE_F("Got temperature data, a=%d b=%d", a, b);
  if (b != TEMP_UNKNOWN) {
    arg->data->temps[arg->dev] = b * 16.0 + a / 16.0;
  }
  return true;
}

int Node::GetTempDevs()
{
  TRACE("Getting temperature device count");

  radio::SendDesc sd;
  sd.to = m_address;
  sd.cmd = GH_CMD_TEMP_DEVS_REQ;
  sd.expectCmd = GH_CMD_TEMP_DEVS_RSP;
  sd.okCallback = &tempDevsOk;
  sd.okCallbackArg = &m_tempData;
  
  m_tempData.devs = common::k_unknown;

  if (Radio().Send(sd)) {
    return m_tempData.devs;
  }
  else {
    return common::k_unknown;
  }
}

float Node::GetTemp(byte index) {
  TRACE_F("Getting temperature value from device: %d", index);

  
  SendDesc sd;
  sd.to = m_address;
  sd.cmd = GH_CMD_TEMP_DATA_REQ;
  sd.expectCmd = GH_CMD_TEMP_DATA_RSP;
  sd.data1 = index;

  TempDataCallbackArg arg;
  arg.dev = index;
  arg.data = &m_tempData;
  sd.okCallbackArg = &arg;
  sd.okCallback = &tempDataOk;
  
  m_tempData.temps[index] = common::k_unknown;
  
  if (Radio().Send(sd)) {
    return m_tempData.temps[index];
  }
  else {
    return common::k_unknown;
  }
}

} // namespace radio

// end Node classs

// begin free functions

#ifdef RADIO_TRACE

void printBuffer(const __FlashStringHelper *prompt, const uint8_t *data, uint8_t len)
{
  char printBuf[100];
  strcpy(printBuf, String(prompt).c_str());
  int printLen = strlen(printBuf);
  for (uint8_t i = 0; i < len; i++) {
    sprintf(printBuf + printLen, "%02X", (unsigned int)data[i]);
    printLen += 2;

    if (i != len - 1) {
      printBuf[printLen++] = ' ';
    }
  }
  printBuf[printLen] = '\0';
  TRACE_C(printBuf);
}

#endif

// end free functions

} // namespace greenhouse
} // namespace embedded

#endif // RADIO_EN
