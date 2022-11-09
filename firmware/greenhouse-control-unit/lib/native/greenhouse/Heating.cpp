#include "Heating.h"

#include "../common/log.h"
#include "../common/common.h"

using namespace common;

namespace native {
namespace greenhouse {

const float k_waterTempMargin = 1;
const float k_soilTempMargin = .2f;
const float k_airTempMargin = 1;
const float k_waterSoilDeltaMin = 5;
const float k_waterAirDeltaMin = 10;

const float k_waterHeaterPowerUse = 3.4;                            // kW
const float k_waterHeaterCostPerKwh = .20f * k_waterHeaterPowerUse; // 20p/kWh

Heating::Heating() :
  m_system(nullptr),
  m_enabled(true),
  m_dayWaterTemperature(k_unknown),
  m_nightWaterTemperature(k_unknown),
  m_daySoilTemperature(k_unknown),
  m_nightSoilTemperature(k_unknown),
  m_dayAirTemperature(k_unknown),
  m_nightAirTemperature(k_unknown),
  m_waterHeaterIsOn(false),
  m_soilHeatingIsOn(false),
  m_airHeatingIsOn(false),
  m_utilitySwitchIsOn(false),
  m_waterHeaterLastUpdate(k_unknownUL),
  m_waterHeaterCostCumulative(0),
  m_waterHeaterDayRuntimeMinutes(0),
  m_waterHeaterDayLimitMinutes(k_unknown),
  m_waterHeaterNightRuntimeMinutes(0),
  m_waterHeaterNightLimitMinutes(k_unknown)
{
}

bool Heating::SwitchWaterHeater(bool on)
{
  if (on) {
    float runtime;
    int limit;

    if (System().Time().IsDaytime()) {
      runtime = WaterHeaterDayRuntimeMinutes();
      limit = WaterHeaterDayLimitMinutes();
    }
    else {
      runtime = WaterHeaterNightRuntimeMinutes();
      limit = WaterHeaterNightLimitMinutes();
    }

    if ((runtime > 0) && (runtime >= limit)) {
      TRACE_F("Blocking water heating switch on, runtime=%.2fm, limit=%dm", runtime, limit);
      return false;
    }
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
  float waterTemp = System().WaterTemperature();
  float soilTemp = System().SoilTemperature();
  float airTemp = System().InsideAirTemperature();

  // ensure that water is warm enough (as not to waste energy)
  bool soilDeltaInBounds = (waterTemp >= soilTemp + k_waterSoilDeltaMin);
  bool airDeltaInBounds = (waterTemp >= airTemp + k_waterAirDeltaMin);

  TRACE_F(
    "Water heater deltas in bounds, soil=%s, air=%s",
    soilDeltaInBounds ? "true" : "false",
    airDeltaInBounds ? "true" : "false");

  bool soilHeatingRequired = SoilHeatingIsOn();
  bool airHeatingRequired = AirHeatingIsOn();

  if (soilTemp != k_unknown) {
    if (soilTemp < (soilTarget - k_soilTempMargin)) {

      TRACE("Soil temp below");
      soilHeatingRequired = true;

      if (soilDeltaInBounds) {
        SwitchSoilHeating(true);
      }
      else {
        SwitchSoilHeating(false);
      }
    }
    else if (soilTemp > (soilTarget + k_soilTempMargin)) {

      TRACE("Soil temp above");
      soilHeatingRequired = false;

      SwitchSoilHeating(false);
    }
  }

  if (airTemp != k_unknown) {
    if (airTemp < (airTarget - k_airTempMargin)) {

      TRACE("Air temp below");
      airHeatingRequired = true;

      if (airDeltaInBounds) {
        SwitchAirHeating(true);
      }
      else {
        SwitchAirHeating(false);
      }
    }
    else if (airTemp > (airTarget + k_airTempMargin)) {

      TRACE("Air temp above");
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

  if (waterTemp < (waterTarget - k_waterTempMargin)) {

    if (airHeatingRequired || soilHeatingRequired) {

      // only switch water heating on if needed
      SwitchWaterHeater(true);
    }
    else {
      // otherwise, turn water heating off (not needed)
      SwitchWaterHeater(false);
    }
  }
  else if (waterTemp > (waterTarget + k_waterTempMargin)) {

    // always switch off, even if soil/air heating on
    SwitchWaterHeater(false);
  }
}

void Heating::Update()
{
  if (System().Time().CurrentHour() == k_unknown) {
    TRACE("Unable to update heating, time unknown");
    return;
  }

  if (!Enabled()) {
    TRACE("Unable to update heating, it is disabled");
    if (AirHeatingIsOn()) {
      SwitchAirHeating(false);
    }
    if (WaterHeaterIsOn()) {
      SwitchWaterHeater(false);
    }
    if (SoilHeatingIsOn()) {
      SwitchSoilHeating(false);
    }
    return;
  }

  bool daytime = System().Time().IsDaytime();
  TRACE_F(
    "Update heating systems, hour=%d, period=%s",
    System().Time().CurrentHour(),
    daytime ? "day" : "night");

  if (WaterHeaterIsOn() && (m_waterHeaterLastUpdate != k_unknownUL)) {

    float runtime;
    int limit;

    if (daytime) {
      runtime = WaterHeaterDayRuntimeMinutes();
      limit = WaterHeaterDayLimitMinutes();
    }
    else {
      runtime = WaterHeaterNightRuntimeMinutes();
      limit = WaterHeaterNightLimitMinutes();
    }

    int addSeconds = (System().Time().UptimeSeconds() - m_waterHeaterLastUpdate);
    runtime += (float)addSeconds / 60;
    m_waterHeaterCostCumulative += (k_waterHeaterCostPerKwh / 3600) * addSeconds; // kWh to kWs

    TRACE_F(
      "Advanced water heating runtime, add=%ds, day=%.2fm, night=%.2fm, cost=£%.2f",
      addSeconds,
      WaterHeaterDayRuntimeMinutes(),
      WaterHeaterNightRuntimeMinutes(),
      WaterHeaterCostCumulative());

    if (runtime >= limit) {
      TRACE_F(
        "Water heater %s runtime limit reached (%dm), switching off",
        daytime ? "day" : "night",
        limit);
      SwitchWaterHeater(false);
    }

    if (daytime) {
      WaterHeaterDayRuntimeMinutes(runtime);
    }
    else {
      WaterHeaterNightRuntimeMinutes(runtime);
    }
  }

  m_waterHeaterLastUpdate = System().Time().UptimeSeconds();

  // heat water to different temperature depending on if day or night
  if (daytime) {
    TRACE("Daytime heating mode");
    UpdatePeriod(DayWaterTemperature(), DaySoilTemperature(), DayAirTemperature());
  }
  else {
    TRACE("Nighttime heating mode");
    UpdatePeriod(NightWaterTemperature(), NightSoilTemperature(), NightAirTemperature());
  }
  
  System().ReportWaterHeaterInfo();
}

void Heating::HandleNightToDayTransition()
{
  WaterHeaterDayRuntimeMinutes(0);
  WaterHeaterCostCumulative(0);
  System().ReportWaterHeaterInfo();
  m_waterHeaterLastUpdate = k_unknownUL;
  TRACE("Water heater day runtime was reset");
}

void Heating::HandleDayToNightTransition()
{
  WaterHeaterNightRuntimeMinutes(0);
  System().ReportWaterHeaterInfo();
  m_waterHeaterLastUpdate = k_unknownUL;
  TRACE("Water heater night runtime was reset");
}

} // namespace greenhouse
} // namespace native
