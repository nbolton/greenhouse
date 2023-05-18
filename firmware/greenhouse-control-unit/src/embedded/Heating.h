#pragma once

#include "native/Heating.h"

#include <stdexcept>
#include <trace.h>

namespace greenhouse {
namespace embedded {

class Heating : public greenhouse::native::Heating {

public:
  Heating();

protected:
  bool SwitchWaterHeater(bool on);
  bool SwitchSoilHeating(bool on);
  bool SwitchAirHeating(bool on);
};

} // namespace embedded
} // namespace greenhouse
