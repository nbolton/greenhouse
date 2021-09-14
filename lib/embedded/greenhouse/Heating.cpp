#include "Heating.h"

#include "ISystem.h"

namespace embedded {
namespace greenhouse {

const int k_fanSwitch = 0;
const int k_pumpSwitch1 = 1;
const int k_pumpSwitch2 = 2;

Heating::Heating() : m_system(nullptr) {}

bool Heating::SwitchWaterHeating(bool on)
{
  if (!native::greenhouse::Heating::SwitchWaterHeating(on)) {
    return false;
  }

  if (on) {
    System().SetSwitch(k_pumpSwitch1, true);
  }
  else {
    System().SetSwitch(k_pumpSwitch1, false);
  }

  return true;
}

bool Heating::SwitchSoilHeating(bool on)
{
  if (!native::greenhouse::Heating::SwitchSoilHeating(on)) {
    return false;
  }

  if (on) {
    System().SetSwitch(k_pumpSwitch2, true);
  }
  else {
    // HACK: until the fan pump arrives, we're sharing the soil heating pump,
    // so only turn off if not in use
    if (!AirHeatingIsOn()) {
      System().SetSwitch(k_pumpSwitch2, false);
    }
  }

  return true;
}

bool Heating::SwitchAirHeating(bool on)
{
  if (!native::greenhouse::Heating::SwitchAirHeating(on)) {
    return false;
  }

  if (on) {
    System().SetSwitch(k_fanSwitch, true);
    System().SetSwitch(k_pumpSwitch2, true);
  }
  else {
    System().SetSwitch(k_fanSwitch, false);

    // HACK: until the fan pump arrives, we're sharing the soil heating pump,
    // so only turn off if not in use
    if (!SoilHeatingIsOn()) {
      System().SetSwitch(k_pumpSwitch2, false);
    }
  }

  return true;
}

} // namespace greenhouse
} // namespace embedded
