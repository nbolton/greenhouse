#pragma once

#include "../../native/greenhouse/Heating.h"
#include <stdexcept>

namespace embedded {
namespace greenhouse {

class ISystem;

class Heating : public native::greenhouse::Heating {

public:
  Heating();

protected:
  bool SwitchWaterHeating(bool on);
  bool SwitchSoilHeating(bool on);
  bool SwitchAirHeating(bool on);

public:
  // getters & setters
  ISystem &System()
  {
    if (m_system != nullptr)
      return *m_system;
    else
      throw std::runtime_error("System not set");
  }
  void System(ISystem &value) { m_system = &value; }

private:
  ISystem *m_system;
};

} // namespace greenhouse
} // namespace embedded
