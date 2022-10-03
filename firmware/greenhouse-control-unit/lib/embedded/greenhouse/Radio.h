#if RADIO_EN

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

typedef bool (*callback)(void *arg);

struct SendDesc {
  byte to = 0;
  byte cmd = 0;
  byte data1 = 0;
  byte data2 = 0;
  byte expectCmd = GH_CMD_ACK;
  callback okCallback = NULL;
  bool okCallbackReturn = false;
  void *okCallbackArg = NULL;
};

class Node;

enum NodeId { k_nodeLeftWindow = 0, k_nodeRightWindow = 1 };

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
  int GetTempDevs();
  float GetTemp(byte index);
  bool MotorRun(MotorDirection direction, byte seconds);
  bool MotorSpeed(byte speed);

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
  embedded::greenhouse::Radio *m_radio;
  TempData m_tempData;
  byte m_address;
};

} // namespace radio

class ISystem;

class Radio {
public:
  Radio();
  void Init();
  void System(ISystem *system) { m_system = system; }
  bool Send(radio::SendDesc sendDesc);
  radio::Node &Node(radio::NodeId index);

private:
  void sr(int pin, bool set);

private:
  ISystem *m_system;
  radio::Node m_nodes[RADIO_NODES_MAX];
};

} // namespace greenhouse
} // namespace embedded

#endif // RADIO_EN
