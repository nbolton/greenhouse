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

const float k_waterHeaterPowerUse = 3.3;                            // kW
const float k_waterHeaterCostPerKwh = .20f * k_waterHeaterPowerUse; // 20p/kWh

Heating::Heating() :
  m_dayWaterTemperature(k_unknown),
  m_nightWaterTemperature(k_unknown),
  m_daySoilTemperature(k_unknown),
  m_nightSoilTemperature(k_unknown),
  m_dayAirTemperature(k_unknown),
  m_nightAirTemperature(k_unknown),
  m_waterHeatingIsOn(false),
  m_soilHeatingIsOn(false),
  m_airHeatingIsOn(false),
  m_waterHeaterLimitMinutes(k_unknown),
  m_waterHeatingLastUpdate(k_unknownUL),
  m_waterHeatingRuntimeMinutes(0),
  m_waterHeatingWasDaytime(false),
  m_waterHeatingHasRun(false),
  m_waterHeatingCostDaily(0)
{
}

bool Heating::SwitchWaterHeating(bool on)
{
  if (
    on && (m_waterHeatingRuntimeMinutes > 0) &&
    (m_waterHeatingRuntimeMinutes >= m_waterHeaterLimitMinutes)) {

    Log().Trace(
      "Blocking water heating switch on, runtime=%.2fm, limit=%dm",
      m_waterHeatingRuntimeMinutes,
      WaterHeaterLimitMinutes());
    return false;
  }

  if (WaterHeatingIsOn() != on) {
    WaterHeatingIsOn(on);
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

void Heating::UpdateDayWaterHeating(bool airHeatingRequired, bool soilHeatingRequired)
{
  if (System().WaterTemperature() < (DayWaterTemperature() - k_waterTempMargin)) {

    if (airHeatingRequired || soilHeatingRequired) {
      // only switch water heating on if needed
      SwitchWaterHeating(true);
    }
    else {
      // otherwise, turn water heating off (not needed)
      SwitchWaterHeating(false);
    }
  }
  else if (System().WaterTemperature() > (DayWaterTemperature() + k_waterTempMargin)) {

    // always switch off, even if soil/air heating on
    SwitchWaterHeating(false);
  }
}

void Heating::UpdateNightWaterHeating(bool airHeatingRequired, bool soilHeatingRequired)
{
  if (System().WaterTemperature() < (NightWaterTemperature() - k_waterTempMargin)) {

    // only switch water heating on if needed
    if (airHeatingRequired || soilHeatingRequired) {
      SwitchWaterHeating(true);
    }
  }
  else if (System().WaterTemperature() > (NightWaterTemperature() + k_waterTempMargin)) {

    // always switch water off if above limit, even if soil/air heating on
    SwitchWaterHeating(false);
  }
}

void Heating::Update()
{
  if (System().CurrentHour() == k_unknown) {
    Log().Trace("Unable to update heating, time unknown");
    return;
  }

  Log().Trace("Update heating systems, hour=%d", System().CurrentHour());

  if (WaterHeatingIsOn() && (m_waterHeatingLastUpdate != k_unknownUL)) {

    int addSeconds = (System().UptimeSeconds() - m_waterHeatingLastUpdate);
    m_waterHeatingRuntimeMinutes += (float)addSeconds / 60;
    m_waterHeatingCostDaily += (k_waterHeaterCostPerKwh / 3600) * addSeconds; // kWh to kWs

    System().ReportWaterHeatingInfo();
    Log().Trace(
      "Advanced water heating runtime, add=%ds, total=%.2fm, cost=£%.2f",
      addSeconds,
      m_waterHeatingRuntimeMinutes,
      m_waterHeatingCostDaily);

    if (m_waterHeatingRuntimeMinutes >= m_waterHeaterLimitMinutes) {
      System().ReportInfo(
        "Water heater runtime limit reached (%dm), switching off", m_waterHeaterLimitMinutes);
      SwitchWaterHeating(false);
    }
  }

  m_waterHeatingLastUpdate = System().UptimeSeconds();

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

  // heat water to different temperature depending on if day or night
  if (
    (System().CurrentHour() >= System().DayStartHour()) &&
    (System().CurrentHour() < System().DayEndHour())) {

    // detect transition from night to day
    if (m_waterHeatingHasRun && !m_waterHeatingWasDaytime) {

      System().HandleNightDayTransition();

      // reset daily runtime and cost back to 0
      WaterHeatingRuntimeMinutes(0);
      WaterHeatingCostDaily(0);
      System().ReportWaterHeatingInfo();
      m_waterHeatingLastUpdate = k_unknownUL;
      Log().Trace("Water heater runtime reset");
    }

    Log().Trace("Daytime heating mode");
    m_waterHeatingWasDaytime = true;

    if ((int)System().SoilTemperature() != k_unknown) {
      if (System().SoilTemperature() < (DaySoilTemperature() - k_soilTempMargin)) {

        Log().Trace("Day soil temp below");
        soilHeatingRequired = true;

        if (soilDeltaInBounds) {
          SwitchSoilHeating(true);
        }
        else {
          SwitchSoilHeating(false);
        }
      }
      else if (System().SoilTemperature() > (DaySoilTemperature() + k_soilTempMargin)) {

        Log().Trace("Day soil temp above");
        soilHeatingRequired = false;

        SwitchSoilHeating(false);
      }
    }

    if ((int)System().InsideAirTemperature() != k_unknown) {
      if (System().InsideAirTemperature() < (DayAirTemperature() - k_airTempMargin)) {

        Log().Trace("Day air temp below");
        airHeatingRequired = true;

        if (soilDeltaInBounds) {
          SwitchAirHeating(true);
        }
        else {
          SwitchAirHeating(false);
        }
      }
      else if (System().InsideAirTemperature() > (DayAirTemperature() + k_airTempMargin)) {

        Log().Trace("Day air temp above");
        airHeatingRequired = false;

        SwitchAirHeating(false);
      }
    }

    UpdateDayWaterHeating(airHeatingRequired, soilHeatingRequired);
  }
  else if (
    (System().CurrentHour() < System().DayStartHour()) ||
    (System().CurrentHour() >= System().DayEndHour())) {

    Log().Trace("Nighttime heating mode");
    m_waterHeatingWasDaytime = false;

    if ((int)System().SoilTemperature() != k_unknown) {
      if (System().SoilTemperature() < (NightSoilTemperature() - k_soilTempMargin)) {

        Log().Trace("Night soil temp below");
        soilHeatingRequired = true;

        if (soilDeltaInBounds) {
          SwitchSoilHeating(true);
        }
        else {
          SwitchSoilHeating(false);
        }
      }
      else if (System().SoilTemperature() > (NightSoilTemperature() + k_soilTempMargin)) {

        Log().Trace("Night soil temp above");
        soilHeatingRequired = false;
        SwitchSoilHeating(false);
      }
    }

    if ((int)System().InsideAirTemperature() != k_unknown) {
      if (System().InsideAirTemperature() < (NightAirTemperature() - k_airTempMargin)) {

        Log().Trace("Night air temp below");
        airHeatingRequired = true;

        if (soilDeltaInBounds) {
          SwitchAirHeating(true);
        }
        else {
          SwitchAirHeating(false);
        }
      }
      else if (System().InsideAirTemperature() > (NightAirTemperature() + k_airTempMargin)) {

        Log().Trace("Night air temp above");
        soilHeatingRequired = false;
        SwitchAirHeating(false);
      }
    }

    UpdateNightWaterHeating(airHeatingRequired, soilHeatingRequired);
  }

  m_waterHeatingHasRun = true;
}

} // namespace greenhouse
} // namespace native
