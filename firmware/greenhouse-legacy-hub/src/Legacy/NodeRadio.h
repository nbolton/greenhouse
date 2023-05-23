#if NODE_RADIO_EN

#pragma once

#include "common.h"

#include <Arduino.h>
#include <gh_protocol.h>

#define TEMP_DEVS_MAX 4
#define RADIO_NODES_MAX 2
#define UNKNOWN_ADDRESS 255

namespace legacy {

class NodeRadio;

struct SendDesc;

typedef bool (*callback)(SendDesc &sendDesc);

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
  void Init(NodeRadio &radio, byte address);
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
  NodeRadio &Radio()
  {
    if (m_radio == nullptr) {
      TRACE("Fatal: Radio not set for node");
      halt();
    }
    return *m_radio;
  }

private:
  bool hello();
  bool keepAlive();
  bool keepAliveExpired();
  void stepSequence();
  bool send(SendDesc &sendDesc);

private:
  bool m_saidHello;
  NodeRadio *m_radio;
  TempData m_tempData;
  byte m_address;
  bool m_helloOk;
  unsigned long m_keepAliveExpiry;
  unsigned long m_nextReconnect;
  uint8_t m_sequence;
  int m_errors;
};

class NodeRadio {
public:
  NodeRadio();
  void Init();
  void Update();
  String DebugInfo();
  bool Send(SendDesc &sendDesc);
  Node &GetNode(NodeId index);
  bool MotorRunAll(MotorDirection direction, byte seconds);
  float GetSoilTemps();
  bool SetWindowSpeeds(int left, int right);

private:
  void sr(int pin, bool set);

private:
  Node m_nodes[RADIO_NODES_MAX];
  int m_requests;
  int m_errors;
};

} // namespace legacy

#endif // NODE_RADIO_EN
