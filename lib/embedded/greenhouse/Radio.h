#pragma once

#include "../Log.h"

namespace embedded {
namespace greenhouse {

enum Signal { Tx, Rx };

class ISystem;

class Radio {
public:
  Radio();
  void Setup();
  void Loop();
  void System(ISystem *system) { m_system = system; }
  bool Busy();

private:
  const embedded::Log &Log() const { return m_log; }

private:
  void sr(int pin, bool set);
  void tx();
  void rx();
  void setSignal(Signal s);
  void errorFlash(int errorFlash);

private:
  embedded::Log m_log;
  ISystem *m_system;
};

} // namespace greenhouse
} // namespace embedded
