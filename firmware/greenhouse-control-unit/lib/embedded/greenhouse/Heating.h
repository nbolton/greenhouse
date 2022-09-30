#pragma once

#include "../../native/greenhouse/Heating.h"
#include "../../common/log.h"

#include <stdexcept>

namespace embedded {
namespace greenhouse {

class Heating : public native::greenhouse::Heating {

public:
  Heating();

protected:
  bool SwitchWaterHeater(bool on);
  bool SwitchSoilHeating(bool on);
  bool SwitchAirHeating(bool on);
  bool SwitchUtility(bool on);
};

} // namespace greenhouse
} // namespace embedded
