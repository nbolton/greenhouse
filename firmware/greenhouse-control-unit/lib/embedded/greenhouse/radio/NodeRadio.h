#if NODE_RADIO_EN

#pragma once

#include "../../common/common.h"
#include "../../common/log.h"

#include <gh_protocol.h>

#define TEMP_DEVS_MAX 4
#define RADIO_NODES_MAX 2
#define UNKNOWN_ADDRESS 255

namespace embedded {
namespace greenhouse {

class Radio;

namespace radio {

struct SendDesc;

typedef bool (*callback)(SendDesc& sendDesc);

struct SendDesc {
  byte to = 0;
  byte cmd = 0;
  byte data1 = 0;
  byte data2 = 0;
  byte seq = 0;
  int errors = 0;
  byte expectCmd = GH_CMD_ACK;
  callback okCallback = NULL;
  bool okCallbackResult = false;
  void *okCallbackArg = NULL;
};

class Node;

enum NodeId { k_nodeRightWindow = 0, k_nodeLeftWindow = 1 };

enum MotorDirection { k_windowExtend = GH_MOTOR_FORWARD, k_windowRetract = GH_MOTOR_REVERSE };

struct TempData {
  int devs;
  float temps[TEMP_DEVS_MAX];
};

struct TempDataCallbackArg {
  TempData *data = NULL;
  int dev = 0;
};

class Node {
public:
  Node();
  void Init(Radio &radio, byte address);
  void Update();
  bool Online();
  int GetTempDevs();
  float GetTemp(byte index);
  bool MotorRun(MotorDirection direction, byte seconds);
  bool MotorSpeed(byte speed);
  bool MotorState(bool &state);
  byte Address() const { return m_address; }
  int Errors() const { return m_errors; }

protected:
  embedded::greenhouse::Radio &Radio()
  {
    if (m_radio == nullptr) {
      TRACE("Fatal: Radio not set for node");
      common::halt();
    }
    return *m_radio;
  }

private:
  bool hello();
  bool keepAlive();
  bool keepAliveExpired();
  void stepSequence();
  bool send(radio::SendDesc &sendDesc);

private:
  bool m_init;
  embedded::greenhouse::Radio *m_radio;
  TempData m_tempData;
  byte m_address;
  bool m_helloOk;
  unsigned long m_keepAliveExpiry;
  unsigned long m_nextReconnect;
  uint8_t m_sequence;
  int m_errors;
};

} // namespace radio

class ISystem;

class Radio {
public:
  Radio();
  void Init(ISystem *system);
  void Update();
  String DebugInfo();
  bool Send(radio::SendDesc &sendDesc);
  radio::Node &Node(radio::NodeId index);
  void MotorRunAll(radio::MotorDirection direction, byte seconds);

private:
  void sr(int pin, bool set);

private:
  ISystem *m_system;
  radio::Node m_nodes[RADIO_NODES_MAX];
  int m_requests;
  int m_errors;
};

} // namespace greenhouse
} // namespace embedded

#endif // NODE_RADIO_EN
