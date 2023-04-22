#pragma once

#if LORA_EN

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

#endif // LORA_EN