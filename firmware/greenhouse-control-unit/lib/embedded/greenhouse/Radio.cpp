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
#define KEEP_ALIVE_TIME 60000 // 60s

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

void printBuffer(const __FlashStringHelper *prompt, const uint8_t *data, uint8_t dataLen);

Radio::Radio() : m_system(nullptr), m_requests(0), m_errors(0) {}

void Radio::Init(ISystem *system)
{
  TRACE("Radio init");
  m_system = system;

  if (!s_driver.init()) {
    TRACE("Fatal: Radio driver init failed");
    common::halt();
  }

  m_nodes[radio::k_nodeRightWindow].Init(*this, GH_ADDR_NODE_1);
  // m_nodes[radio::k_nodeLeftWindow].Init(*this, GH_ADDR_NODE_2);
}

void Radio::Update()
{
  for (int i = 0; i < RADIO_NODES_MAX; i++) {
    m_nodes[i].Update();
  }
}

radio::Node &Radio::Node(radio::NodeId index)
{
  if (index < 0 || index > RADIO_NODES_MAX) {
    TRACE_F("Fatal: Radio node out of bounds, index=%d", (int)index);
    common::halt();
  }
  return m_nodes[index];
}

void Radio::sr(int pin, bool set) { m_system->ShiftRegister(pin, set); }

void stepSequence()
{
  if (s_sequence == 255) {
    s_sequence = 0;
  }
  else {
    s_sequence++;
  }
}

bool Radio::Send(radio::SendDesc &sendDesc)
{
  bool rx = false;
  for (int i = 0; i < TX_RETRY_MAX; i++) {

    TRACE_F(
      "Radio sending, attempt=%d, to=%02X, cmd=%02X, seq=%d",
      i + 1,
      sendDesc.to,
      sendDesc.cmd,
      s_sequence);

    sr(SR_PIN_EN_TX, true);
    delay(TX_WAIT_DELAY);

    GH_TO(s_txBuf) = sendDesc.to;
    GH_FROM(s_txBuf) = GH_ADDR_MAIN;
    GH_CMD(s_txBuf) = sendDesc.cmd;
    GH_DATA_1(s_txBuf) = sendDesc.data1;
    GH_DATA_2(s_txBuf) = sendDesc.data2;

    s_sequence = s_sequence % 256;
    GH_SEQ(s_txBuf) = s_sequence;

    m_requests++;

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
            TRACE_F("Error: Code from node: %d", GH_DATA_1(s_rxBuf));
            m_errors++;
          }
          else if (GH_CMD(s_rxBuf) != sendDesc.expectCmd) {
            TRACE("Error: Radio got unexpected command");
            m_errors++;
          }
          else if (sendDesc.okCallback != NULL) {
            sendDesc.okCallbackResult = sendDesc.okCallback(sendDesc.okCallbackArg);
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
      TRACE("Error: Radio timeout");
      m_errors++;
    }
  }

  stepSequence();

  TRACE_F("Radio stats, rx=%s, errors=%d, requests=%d", BOOL_FS(rx), m_errors, m_requests);

  return rx;
}

void Radio::MotorRunAll(radio::MotorDirection direction, byte seconds)
{
  // const int windowNodes = 2;
  const int windowNodes = 1;
  const int motorBusyWait = 1000;
  const int busyCheckMax = 60;

  radio::NodeId nodes[windowNodes];
  nodes[0] = radio::k_nodeRightWindow;
  // nodes[1] = m_radio.Node(radio::k_nodeLeftWindow);

  for (int i = 0; i < windowNodes; i++) {
    radio::Node &node = Node(nodes[i]);

    int busyChecks = 0;
    bool motorBusy = false, motorStateRx = false;
    do {
      if (busyChecks++ >= busyCheckMax) {
        TRACE_F("Error: Radio motor run busy checks exceeded maximum, node=%02X", node.Address());
        break;
      }
      motorStateRx = node.MotorState(motorBusy);
      if (!motorStateRx) {
        break;
      }
      if (motorBusy) {
        TRACE("Motor is busy, waiting");
        m_system->Delay(motorBusyWait, "Motor busy wait");
      }
    } while (motorBusy);

    if (motorStateRx && !motorBusy) {
      node.MotorRun(direction, seconds);
    }
  }
}

// begin node class

namespace radio {

Node::Node() :
  m_radio(nullptr),
  m_address(UNKNOWN_ADDRESS),
  m_helloOk(false),
  m_keepAliveExpiry(common::k_unknownUL)
{
}

void Node::Init(embedded::greenhouse::Radio &radio, byte address)
{
  TRACE_F("Init radio node, address=%02X", address);
  m_radio = &radio;
  m_address = address;
  if (hello()) {
    TRACE_F("Radio node online: %02X", m_address);
  }
  else {
    TRACE_F("Radio node offline: %02X", m_address);
  }
}

void Node::Update() { keepAlive(); }

bool Node::Online() { return m_helloOk && !keepAliveExpired(); }

bool Node::keepAliveExpired() { return millis() > m_keepAliveExpiry; }

bool Node::keepAlive()
{
  if (keepAliveExpired()) {
    TRACE_F("Radio keep alive expired, saying hello to node: %02X", m_address);
    if (!hello()) {
      TRACE_F("Error: Radio keep alive failed, node offline: %02X", m_address);
      return false;
    }
  }
  return m_helloOk;
}

bool helloOk(void *arg)
{
  const int rxSeq = GH_SEQ(s_rxBuf);
  if (rxSeq != s_sequence) {
    TRACE_F("Error: Radio ack invalid, sequence: %d!=%d", rxSeq, s_sequence);
    return false;
  }

  return true;
}

bool Node::hello()
{
  TRACE_F("Radio saying hello to %02X", m_address);

  SendDesc sd;
  sd.to = GH_ADDR_NODE_1;
  sd.cmd = GH_CMD_HELLO;
  sd.okCallback = &helloOk;
  bool rxOk = Radio().Send(sd);

  m_helloOk = rxOk && sd.okCallbackResult;
  if (m_helloOk) {
    m_keepAliveExpiry = millis() + KEEP_ALIVE_TIME;
  }
  else {
    TRACE_F(
      "Error: Radio hello failed, rx=%s, ack=%s", BOOL_FS(rxOk), BOOL_FS(sd.okCallbackResult));
  }

  return m_helloOk;
}

bool tempDevsOk(void *arg)
{
  radio::TempData *data = (radio::TempData *)arg;
  data->devs = GH_DATA_1(s_rxBuf);
  TRACE_F("Got temperature device count: %d", data->devs);
  return true;
}

int Node::GetTempDevs()
{
  if (!keepAlive()) {
    return common::k_unknown;
  }

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

bool tempDataOk(void *argV)
{
  TempDataCallbackArg *arg = (TempDataCallbackArg *)argV;
  const uint8_t a = GH_DATA_1(s_rxBuf);
  const uint8_t b = GH_DATA_2(s_rxBuf);
  TRACE_F("Got temperature data, a=%02X b=%02X", a, b);
  if (b != TEMP_UNKNOWN) {
    arg->data->temps[arg->dev] = b * 16.0 + a / 16.0;
  }
  return true;
}

float Node::GetTemp(byte index)
{
  if (!keepAlive()) {
    return common::k_unknown;
  }

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

bool Node::MotorRun(MotorDirection direction, byte seconds)
{
  if (!keepAlive()) {
    return false;
  }

  TRACE_F("Radio sending motor run: direction=%d seconds=%d", direction, seconds);
  SendDesc sd;
  sd.to = m_address;
  sd.cmd = GH_CMD_MOTOR_RUN;
  sd.data1 = (byte)direction;
  sd.data2 = seconds;
  return Radio().Send(sd);
}

bool Node::MotorSpeed(byte speed)
{
  if (!keepAlive()) {
    return false;
  }

  TRACE_F("Radio sending motor speed: speed=%d", speed);
  SendDesc sd;
  sd.to = m_address;
  sd.cmd = GH_CMD_MOTOR_SPEED;
  sd.data1 = speed;
  return Radio().Send(sd);
}

bool Node::MotorState(bool &state)
{
  if (!keepAlive()) {
    return false;
  }

  TRACE("Radio requesting motor state");
  SendDesc sd;
  sd.to = m_address;
  sd.cmd = GH_CMD_MOTOR_STATE_REQ;
  sd.expectCmd = GH_CMD_MOTOR_STATE_RSP;
  if (Radio().Send(sd)) {
    state = GH_DATA_1(s_rxBuf);
    TRACE_F("Radio got motor state, running=%s", BOOL_FS(state));
    return true;
  }
  return false;
}

} // namespace radio

// end Node classs

// begin free functions

#ifdef RADIO_TRACE

void printBuffer(const __FlashStringHelper *prompt, const uint8_t *data, uint8_t dataLen)
{
  char printBuf[100];
  strcpy(printBuf, String(prompt).c_str());
  int printLen = strlen(printBuf);
  for (uint8_t i = 0; i < dataLen; i++) {
    sprintf(printBuf + printLen, "%02X", (unsigned int)data[i]);
    printLen += 2;

    if (i != dataLen - 1) {
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
