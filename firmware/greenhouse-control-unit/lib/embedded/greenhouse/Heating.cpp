#include "Heating.h"

#include <Arduino.h>

namespace embedded {
namespace greenhouse {

//#define PUMPS_EN

const int k_fanSwitch = 0;
#ifdef PUMPS_EN
const int k_pumpSwitch1 = 1;
const int k_pumpSwitch2 = 2;
#endif // PUMPS_EN
const int k_utilitySwitch = 3;

Heating::Heating() {}

bool Heating::SwitchWaterHeater(bool on)
{
  if (!native::greenhouse::Heating::SwitchWaterHeater(on)) {
    return false;
  }

#ifdef PUMPS_EN
  if (on) {
    System().SetSwitch(k_pumpSwitch1, true);
  }
  else {
    System().SetSwitch(k_pumpSwitch1, false);
  }
#endif // PUMPS_EN

  return true;
}

bool Heating::SwitchSoilHeating(bool on)
{
  if (!native::greenhouse::Heating::SwitchSoilHeating(on)) {
    return false;
  }

#ifdef PUMPS_EN
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
#endif // PUMPS_EN

  return true;
}

bool Heating::SwitchAirHeating(bool on)
{
  if (!native::greenhouse::Heating::SwitchAirHeating(on)) {
    return false;
  }

#ifdef PUMPS_EN
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
#endif // PUMPS_EN

  return true;
}

bool Heating::SwitchUtility(bool on)
{
  if (!native::greenhouse::Heating::SwitchUtility(on)) {
    return false;
  }

  System().SetSwitch(k_utilitySwitch, on);
  return true;
}

} // namespace greenhouse
} // namespace embedded
