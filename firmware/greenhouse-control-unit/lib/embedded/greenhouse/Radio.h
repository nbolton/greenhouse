#if RADIO_EN

#pragma once

#include "../../common/common.h"
#include "../../common/log.h"

#include <gh_protocol.h>

#define TEMP_DEVS_MAX 4
#define NODES_MAX 2
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
  Node() : m_address(UNKNOWN_ADDRESS) {}
  int GetTempDevs();
  float GetTemp(byte index);
  void Radio(Radio &radio) { m_radio = &radio; }
  void Address(byte address) { m_address = address; }

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
  void Setup();
  void System(ISystem *system) { m_system = system; }
  bool Send(radio::SendDesc &sendDesc);
  const radio::Node &Node(radio::NodeId index) const { return m_nodes[index]; }

private:
  void sr(int pin, bool set);

private:
  ISystem *m_system;
  radio::Node m_nodes[NODES_MAX];
};

} // namespace greenhouse
} // namespace embedded

#endif // RADIO_EN
