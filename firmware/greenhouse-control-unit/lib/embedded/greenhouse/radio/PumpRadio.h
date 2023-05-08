#pragma once

#if PUMP_RADIO_EN

#include "ISystem.h"

namespace embedded {
namespace greenhouse {

class PumpRadio {
public:
  PumpRadio();
  void Init(embedded::greenhouse::ISystem *system);
  void Update();
  void SendPumpMessage();
  void SwitchPump(bool on);

private:
  embedded::greenhouse::ISystem *m_system;
};

} // namespace greenhouse
} // namespace embedded

#endif // PUMP_RADIO_EN