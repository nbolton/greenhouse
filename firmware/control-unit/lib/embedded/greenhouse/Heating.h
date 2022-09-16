#pragma once

#include "../../native/greenhouse/Heating.h"
#include "../embedded/Log.h"

#include <stdexcept>

namespace embedded {
namespace greenhouse {

class Heating : public native::greenhouse::Heating {

public:
  Heating();
  const embedded::Log &Log() const { return m_log; }

protected:
  bool SwitchWaterHeater(bool on);
  bool SwitchSoilHeating(bool on);
  bool SwitchAirHeating(bool on);
  bool SwitchUtility(bool on);

private:
  embedded::Log m_log;
};

} // namespace greenhouse
} // namespace embedded
