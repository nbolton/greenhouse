#include "Heating.h"

#include "../common/Log.h"
#include "../common/common.h"

using namespace common;

namespace native {
namespace greenhouse {

const float k_waterTempMargin = 1;
const float k_soilTempMargin = .2f;
const float k_airTempMargin = 1;
const float k_minimumWaterDelta = 5;

const float k_waterHeaterPowerUse = 3.4;                            // kW
const float k_waterHeaterCostPerKwh = .20f * k_waterHeaterPowerUse; // 20p/kWh

Heating::Heating() :
  m_system(nullptr),
  m_dayWaterTemperature(k_unknown),
  m_nightWaterTemperature(k_unknown),
  m_daySoilTemperature(k_unknown),
  m_nightSoilTemperature(k_unknown),
  m_dayAirTemperature(k_unknown),
  m_nightAirTemperature(k_unknown),
  m_waterHeaterIsOn(false),
  m_soilHeatingIsOn(false),
  m_airHeatingIsOn(false),
  m_waterHeaterRuntimeMinutes(0),
  m_waterHeaterLimitMinutes(k_unknown),
  m_waterHeaterLastUpdate(k_unknownUL),
  m_waterHeaterCostCumulative(0)
{
}

bool Heating::SwitchWaterHeater(bool on)
{
  if (
    on && (WaterHeaterRuntimeMinutes() > 0) &&
    (WaterHeaterRuntimeMinutes() >= WaterHeaterLimitMinutes())) {

    Log().Trace(
      "Blocking water heating switch on, runtime=%.2fm, limit=%dm",
      WaterHeaterRuntimeMinutes(),
      WaterHeaterLimitMinutes());
    return false;
  }

  if (WaterHeaterIsOn() != on) {
    WaterHeaterIsOn(on);
    return true;
  }

  return false;
}

bool Heating::SwitchSoilHeating(bool on)
{
  if (SoilHeatingIsOn() != on) {
    SoilHeatingIsOn(on);
    return true;
  }

  return false;
}

bool Heating::SwitchAirHeating(bool on)
{
  if (AirHeatingIsOn() != on) {
    AirHeatingIsOn(on);
    return true;
  }

  return false;
}

void Heating::UpdatePeriod(float waterTarget, float soilTarget, float airTarget)
{
  // ensure that water is warm enough (as not to waste energy)
  bool soilDeltaInBounds =
    (System().WaterTemperature() >= System().SoilTemperature() + k_minimumWaterDelta);
  bool airDeltaInBounds =
    (System().WaterTemperature() >= System().InsideAirTemperature() + k_minimumWaterDelta);

  Log().Trace(
    "Water heater deltas in bounds, soil=%s, air=%s",
    soilDeltaInBounds ? "true" : "false",
    airDeltaInBounds ? "true" : "false");

  bool soilHeatingRequired = SoilHeatingIsOn();
  bool airHeatingRequired = AirHeatingIsOn();

  if (System().SoilTemperature() != k_unknown) {
    if (System().SoilTemperature() < (soilTarget - k_soilTempMargin)) {

      Log().Trace("Soil temp below");
      soilHeatingRequired = true;

      if (soilDeltaInBounds) {
        SwitchSoilHeating(true);
      }
      else {
        SwitchSoilHeating(false);
      }
    }
    else if (System().SoilTemperature() > (soilTarget + k_soilTempMargin)) {

      Log().Trace("Soil temp above");
      soilHeatingRequired = false;

      SwitchSoilHeating(false);
    }
  }

  if (System().InsideAirTemperature() != k_unknown) {
    if (System().InsideAirTemperature() < (airTarget - k_airTempMargin)) {

      Log().Trace("Air temp below");
      airHeatingRequired = true;

      if (airDeltaInBounds) {
        SwitchAirHeating(true);
      }
      else {
        SwitchAirHeating(false);
      }
    }
    else if (System().InsideAirTemperature() > (airTarget + k_airTempMargin)) {

      Log().Trace("Air temp above");
      airHeatingRequired = false;

      SwitchAirHeating(false);
    }
  }

  if (!soilDeltaInBounds) {
    SwitchSoilHeating(false);
  }

  if (!airDeltaInBounds) {
    SwitchAirHeating(false);
  }

  if (System().WaterTemperature() < (waterTarget - k_waterTempMargin)) {

    if (airHeatingRequired || soilHeatingRequired) {

      // only switch water heating on if needed
      SwitchWaterHeater(true);
    }
    else {
      // otherwise, turn water heating off (not needed)
      SwitchWaterHeater(false);
    }
  }
  else if (System().WaterTemperature() > (waterTarget + k_waterTempMargin)) {

    // always switch off, even if soil/air heating on
    SwitchWaterHeater(false);
  }
}

void Heating::Update()
{
  if (System().CurrentHour() == k_unknown) {
    Log().Trace("Unable to update heating, time unknown");
    return;
  }

  Log().Trace("Update heating systems, hour=%d", System().CurrentHour());

  if (WaterHeaterIsOn() && (m_waterHeaterLastUpdate != k_unknownUL)) {

    int addSeconds = (System().UptimeSeconds() - m_waterHeaterLastUpdate);
    m_waterHeaterRuntimeMinutes += (float)addSeconds / 60;
    m_waterHeaterCostCumulative += (k_waterHeaterCostPerKwh / 3600) * addSeconds; // kWh to kWs

    System().ReportWaterHeaterInfo();
    Log().Trace(
      "Advanced water heating runtime, add=%ds, total=%.2fm, cost=£%.2f",
      addSeconds,
      m_waterHeaterRuntimeMinutes,
      m_waterHeaterCostCumulative);

    if (m_waterHeaterRuntimeMinutes >= m_waterHeaterLimitMinutes) {
      System().ReportInfo(
        "Water heater runtime limit reached (%dm), switching off", m_waterHeaterLimitMinutes);
      SwitchWaterHeater(false);
    }
  }

  m_waterHeaterLastUpdate = System().UptimeSeconds();

  // heat water to different temperature depending on if day or night
  if (System().IsDaytime()) {
    Log().Trace("Daytime heating mode");
    UpdatePeriod(DayWaterTemperature(), DaySoilTemperature(), DayAirTemperature());
  }
  else {
    Log().Trace("Nighttime heating mode");
    UpdatePeriod(NightWaterTemperature(), NightSoilTemperature(), NightAirTemperature());
  }
}

void Heating::HandleDayNightTransition()
{
  // reset daily runtime and cost back to 0
  WaterHeaterRuntimeMinutes(0);
  WaterHeaterCostCumulative(0);
  System().ReportWaterHeaterInfo();
  m_waterHeaterLastUpdate = k_unknownUL;
  Log().Trace("Water heater runtime was reset");
}

} // namespace greenhouse
} // namespace native
