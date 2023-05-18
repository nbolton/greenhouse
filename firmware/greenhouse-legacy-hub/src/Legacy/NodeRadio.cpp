#include "NodeRadio.h"

#if NODE_RADIO_EN

#include "common.h"

#include <SoftwareSerial.h>

#define PIN_RX PC15
#define PIN_TX PC14
#define BAUD 9600
#define RX_TIMEOUT 200
#define TX_WAIT_DELAY 20
#define TX_RETRY_MAX 10
// #define LINEAR_TIMEOUT 1
#define TEMP_OFFSET -1.2
#define TEMP_UNKNOWN 255
#define KEEP_ALIVE_TIME 60000 // 60s
#define RECONNECT_TIME 30000  // 30s

namespace legacy {

static SoftwareSerial s_hc12(PIN_TX, PIN_RX);
static uint8_t s_rxBuf[GH_LENGTH];
static uint8_t s_txBuf[GH_LENGTH];

void printBuffer(const __FlashStringHelper *prompt, const uint8_t *data, uint8_t dataLen);

NodeRadio::NodeRadio() : m_requests(0), m_errors(0) {}

void NodeRadio::Init()
{
  TRACE("Radio init");

  s_hc12.begin(BAUD);

  m_nodes[k_nodeRightWindow].Init(*this, GH_ADDR_NODE_1);
  m_nodes[k_nodeLeftWindow].Init(*this, GH_ADDR_NODE_2);
}

void NodeRadio::Update()
{
  for (int i = 0; i < RADIO_NODES_MAX; i++) {
    m_nodes[i].Update();
  }
}

String NodeRadio::DebugInfo()
{
  String debug;
  for (int i = 0; i < RADIO_NODES_MAX; i++) {
    Node &node = m_nodes[i];
    char buf[200];
    String f = String(F("%d:{addr=%02Xh, err=%d} "));
    sprintf(buf, f.c_str(), i, node.Address(), node.Errors());
    debug += buf;
  }
  return debug;
}

Node &NodeRadio::GetNode(NodeId index)
{
  if (index < 0 || index >= RADIO_NODES_MAX) {
    TRACE_F("Fatal: Radio node out of bounds, index=%d", (int)index);
    halt();
  }
  return m_nodes[index];
}

void NodeRadio::sr(int pin, bool set)
{ /*WriteOnboardIO(pin, set);*/
}

bool NodeRadio::Send(SendDesc &sendDesc)
{
  led(LOW);
  bool rx = false;
  for (int i = 0; i < TX_RETRY_MAX; i++) {

    TRACE_F(
      "Radio sending, attempt=%d, to=%02Xh, cmd=%02Xh, seq=%d",
      i + 1,
      sendDesc.to,
      sendDesc.cmd,
      sendDesc.seq);

    GH_TO(s_txBuf) = sendDesc.to;
    GH_FROM(s_txBuf) = GH_ADDR_MAIN;
    GH_CMD(s_txBuf) = sendDesc.cmd;
    GH_DATA_1(s_txBuf) = sendDesc.data1;
    GH_DATA_2(s_txBuf) = sendDesc.data2;

    sendDesc.seq = sendDesc.seq % 256;
    GH_SEQ(s_txBuf) = sendDesc.seq;

    m_requests++;

    // clear anything left in the RX buffer, otherwise when we're
    // checking for a response, we may get an out of sync message.
    int bitsDumped = 0;
    while (s_hc12.read() != -1) {
      bitsDumped++;
    }

    TRACE_F("Radio read bits dumped: %d", bitsDumped);

    s_hc12.write(s_txBuf, GH_LENGTH);

    unsigned long start = millis();

    printBuffer(F("Radio sent data: "), s_txBuf, GH_LENGTH);

    int timeout = RX_TIMEOUT;
#if LINEAR_TIMEOUT
    // linear timeout increase; 433MHz is a very busy frequency,
    // and we don't want to fight with another retry loop.
    // don't wait for long on the last retry.
    if ((i != 0) && (i != (TX_RETRY_MAX - 1))) {
      TRACE("Radio response timout increased");
      timeout *= i + 1;
    }
#endif // LINEAR_TIMEOUT
    TRACE_F("Radio waiting for response, timeout: %dms", timeout);

    while (millis() < (start + timeout)) {
      uint8_t s_rxBufLen = sizeof(s_rxBuf);

      if (s_hc12.available()) {
        s_rxBufLen = s_hc12.readBytes(s_rxBuf, GH_LENGTH);
        if (s_rxBufLen != GH_LENGTH) {
          TRACE_F(
            "Error: Buffer underrun while waiting for %02Xh: %d", sendDesc.to, GH_DATA_1(s_rxBuf));
          m_errors++;
          sendDesc.errors++;
          break;
        }

        TRACE_F("Radio response time: %lums", millis() - start);
        printBuffer(F("Radio got data: "), s_rxBuf, GH_LENGTH);

        if ((GH_TO(s_rxBuf) == GH_ADDR_MAIN) && (GH_FROM(s_rxBuf) == sendDesc.to)) {

          if (GH_CMD(s_rxBuf) == GH_CMD_ERROR) {
            TRACE_F("Error: Code from node %02Xh: %d", sendDesc.to, GH_DATA_1(s_rxBuf));
            m_errors++;
            sendDesc.errors++;
            continue;
          }
          else if (GH_CMD(s_rxBuf) != sendDesc.expectCmd) {
            TRACE("Error: Radio got unexpected command");
            m_errors++;
            sendDesc.errors++;
            continue;
          }
          else if (sendDesc.okCallback != NULL) {
            sendDesc.okCallbackResult = sendDesc.okCallback(sendDesc);
            if (!sendDesc.okCallbackResult) {
              TRACE("Error: Radio OK callback failed");
              m_errors++;
              sendDesc.errors++;
              continue;
            }
          }

          rx = true;
          break;
        }
        else {
          TRACE("Radio ignoring message (address mismatch)");
          // don't report an error, this is fine
        }
      }
    }

    if (rx) {
      break;
    }
    else {
      TRACE("Error: Radio timeout");
      m_errors++;
      sendDesc.errors++;
    }
  }

  TRACE_F(
    "Radio stats, rx=%s, errors={now: %d, total: %d}, requests=%d",
    BOOL_FS(rx),
    sendDesc.errors,
    m_errors,
    m_requests);

  led(HIGH);
  return rx;
}

void NodeRadio::MotorRunAll(MotorDirection direction, byte seconds)
{
  const int windowNodes = 2;
  const int motorBusyWait = 1000;
  const int busyCheckMax = 60;

  NodeId nodes[windowNodes];
  nodes[0] = k_nodeRightWindow;
  nodes[1] = k_nodeLeftWindow;

  for (int i = 0; i < windowNodes; i++) {
    Node &node = GetNode(nodes[i]);

    int busyChecks = 0;
    bool motorBusy = false, motorStateRx = false;
    do {
      if (busyChecks++ >= busyCheckMax) {
        TRACE_F("Error: Radio motor run busy checks exceeded maximum, node=%02Xh", node.Address());
        break;
      }
      motorStateRx = node.MotorState(motorBusy);
      if (!motorStateRx) {
        break;
      }
      if (motorBusy) {
        TRACE("Motor is busy, waiting");
        delay(motorBusyWait);
      }
    } while (motorBusy);

    if (motorStateRx && !motorBusy) {
      node.MotorRun(direction, seconds);
    }
  }
}

#define SOIL_TEMP_FAIL_MAX 10

float NodeRadio::GetSoilTemps()
{
  Node &rightWindow = GetNode(k_nodeRightWindow);
  const int tempDevs = rightWindow.GetTempDevs();

  TRACE_F("Soil temperature devices: %d", tempDevs);
  float tempSum = 0;
  int tempValues = 0;
  for (int i = 0; i < tempDevs; i++) {
    const float t = rightWindow.GetTemp(i);
    if (t != k_unknown) {
      tempSum += t;
      tempValues++;
      TRACE_F("Soil temperature sensor %d: %.2f", i, t);
    }
    else {
      TRACE_F("Soil temperature sensor %d: unknown", i);
    }
  }

  int soilTemperatureFailures = 0;
  float soilTemperature;
  if (tempValues > 0) {
    soilTemperatureFailures = 0;
    soilTemperature = tempSum / tempValues;
    TRACE_F("Soil temperature average: %.2f", m_soilTemperature);
  }
  else if (++soilTemperatureFailures > SOIL_TEMP_FAIL_MAX) {
    // only report -1 if there are a number of failures
    soilTemperatureFailures = 0;
    soilTemperature = k_unknown;
    TRACE("Soil temperatures unknown");
  }
  return soilTemperature;
}

void NodeRadio::SetWindowSpeeds(int left, int right)
{
  Node &leftNode = GetNode(k_nodeLeftWindow);
  Node &rightNode = GetNode(k_nodeRightWindow);
  leftNode.MotorSpeed(left);
  rightNode.MotorSpeed(right);
}

// begin node class

Node::Node() :
  m_saidHello(false),
  m_radio(nullptr),
  m_address(UNKNOWN_ADDRESS),
  m_helloOk(false),
  m_keepAliveExpiry(k_unknownUL),
  m_nextReconnect(k_unknownUL),
  m_sequence(1),
  m_errors(0)
{
}

void Node::Init(NodeRadio &radio, byte address)
{
  TRACE_F("Init radio node, address=%02Xh", address);
  m_radio = &radio;
  m_address = address;
}

void Node::Update()
{
  if (!m_saidHello) {
    if (hello()) {
      TRACE_F("Radio node online: %02Xh", m_address);
    }
    else {
      TRACE_F("Radio node offline: %02Xh", m_address);
    }
    m_saidHello = true;
  }

  if (m_helloOk) {
    keepAlive();
  }
  else {
    if (millis() > m_nextReconnect) {
      TRACE_F("Reconnecting to node: %02Xh", m_address);
      if (hello()) {
        TRACE_F("Radio node online: %02Xh", m_address);
      }
      else {
        TRACE_F("Failed to reconnect to node: %02Xh", m_address);
      }
    }
  }
}

bool Node::Online() { return m_helloOk && !keepAliveExpired(); }

void Node::stepSequence()
{
  if (m_sequence == 255) {
    m_sequence = 0;
  }
  else {
    m_sequence++;
  }
}

bool Node::keepAliveExpired() { return millis() > m_keepAliveExpiry; }

bool Node::keepAlive()
{
  if (keepAliveExpired()) {
    TRACE_F("Radio keep alive expired, saying hello to node: %02Xh", m_address);
    if (!hello()) {
      TRACE_F("Error: Radio keep alive failed, node offline: %02Xh", m_address);
      return false;
    }
  }
  return m_helloOk;
}

bool helloOk(SendDesc &sendDesc)
{
  const byte rxSeq = GH_SEQ(s_rxBuf);
  if (rxSeq != sendDesc.seq) {
    TRACE_F("Error: Radio ack invalid, sequence: %d!=%d", rxSeq, sendDesc.seq);
    return false;
  }

  return true;
}

bool Node::send(SendDesc &sendDesc)
{
  sendDesc.seq = m_sequence;
  bool ok = Radio().Send(sendDesc);
  m_errors += sendDesc.errors;
  TRACE_F("Total errors for node %02Xh: %d", m_address, m_errors);
  stepSequence();
  return ok;
}

bool Node::hello()
{
  TRACE_F("Radio saying hello to %02Xh", m_address);

  SendDesc sd;
  sd.to = m_address;
  sd.cmd = GH_CMD_HELLO;
  sd.okCallback = &helloOk;
  bool rxOk = send(sd);

  m_helloOk = rxOk && sd.okCallbackResult;
  if (m_helloOk) {
    m_keepAliveExpiry = millis() + KEEP_ALIVE_TIME;
  }
  else {
    m_nextReconnect = millis() + RECONNECT_TIME;
    TRACE_F(
      "Error: Radio hello failed, rx=%s, ack=%s", BOOL_FS(rxOk), BOOL_FS(sd.okCallbackResult));
  }

  return m_helloOk;
}

bool tempDevsOk(SendDesc &sendDesc)
{
  TempData *data = (TempData *)sendDesc.okCallbackArg;
  data->devs = GH_DATA_1(s_rxBuf);
  TRACE_F("Got temperature device count: %d", data->devs);
  return true;
}

int Node::GetTempDevs()
{
  if (!keepAlive()) {
    return k_unknown;
  }

  TRACE("Getting temperature device count");

  SendDesc sd;
  sd.to = m_address;
  sd.cmd = GH_CMD_TEMP_DEVS_REQ;
  sd.expectCmd = GH_CMD_TEMP_DEVS_RSP;
  sd.okCallback = &tempDevsOk;
  sd.okCallbackArg = &m_tempData;

  m_tempData.devs = k_unknown;

  if (send(sd)) {
    return m_tempData.devs;
  }
  else {
    return k_unknown;
  }
}

bool tempDataOk(SendDesc &sendDesc)
{
  TempDataCallbackArg *arg = (TempDataCallbackArg *)sendDesc.okCallbackArg;
  const uint8_t a = GH_DATA_1(s_rxBuf);
  const uint8_t b = GH_DATA_2(s_rxBuf);
  TRACE_F("Got temperature data, a=%02Xh b=%02Xh", a, b);
  if (b != TEMP_UNKNOWN) {
    arg->data->temps[arg->dev] = b * 16.0 + a / 16.0;
  }
  return true;
}

float Node::GetTemp(byte index)
{
  if (!keepAlive()) {
    return k_unknown;
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

  m_tempData.temps[index] = k_unknown;

  if (send(sd)) {
    return m_tempData.temps[index];
  }
  else {
    return k_unknown;
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
  return send(sd);
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
  return send(sd);
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
  if (send(sd)) {
    state = GH_DATA_1(s_rxBuf);
    TRACE_F("Radio got motor state, running=%s", BOOL_FS(state));
    return true;
  }
  return false;
}

// end Node class

// begin free functions

void printBuffer(const __FlashStringHelper *prompt, const uint8_t *data, uint8_t dataLen)
{
#ifdef RADIO_TRACE
  char printBuf[100];
  strcpy(printBuf, String(prompt).c_str());
  int printLen = strlen(printBuf);
  for (uint8_t i = 0; i < dataLen; i++) {
    sprintf(printBuf + printLen, "%02Xh", (unsigned int)data[i]);
    printLen += 2;

    if (i != dataLen - 1) {
      printBuf[printLen++] = ' ';
    }
  }
  printBuf[printLen] = '\0';
  TRACE_C(printBuf);
#endif
}

// end free functions

} // namespace legacy

#endif // NODE_RADIO_EN
