#if RADIO_EN

#pragma once

#include "../../common/log.h"

namespace embedded {
namespace greenhouse {

enum Signal { Idle, Tx, Rx };

enum Mode {
  Ping,
  Zing,
  Pong,
};

class ISystem;

class Radio {
public:
  Radio();
  void Setup();
  void Loop();
  void System(ISystem *system) { m_system = system; }
  bool Busy();

private:
  void sr(int pin, bool set);
  void tx();
  void rx();
  void setSignal(Signal s);
  void setMode(Mode _mode);
  void errorFlash(int times);
  void toggleMode();

private:
  ISystem *m_system;
};

} // namespace greenhouse
} // namespace embedded

#endif // RADIO_EN
