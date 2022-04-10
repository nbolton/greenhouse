#pragma once

#include "ISystem.h"
#include "Log.h"

#include <stdexcept>

namespace native {
namespace greenhouse {

class Heating {

public:
  Heating();
  void Update();
  const common::Log &Log() const { return m_log; }
  void HandleNightToDayTransition();
  void HandleDayToNightTransition();

protected:
  virtual bool SwitchWaterHeater(bool on);
  virtual bool SwitchSoilHeating(bool on);
  virtual bool SwitchAirHeating(bool on);

public:
  // getters & setters
  virtual bool Enabled() const { return m_enabled; }
  virtual void Enabled(bool value) { m_enabled = value; }
  virtual void DayWaterTemperature(float value) { m_dayWaterTemperature = value; }
  virtual float DayWaterTemperature() const { return m_dayWaterTemperature; }
  virtual void NightWaterTemperature(float value) { m_nightWaterTemperature = value; }
  virtual float NightWaterTemperature() const { return m_nightWaterTemperature; }
  virtual void DaySoilTemperature(float value) { m_daySoilTemperature = value; }
  virtual float DaySoilTemperature() const { return m_daySoilTemperature; }
  virtual void NightSoilTemperature(float value) { m_nightSoilTemperature = value; }
  virtual float NightSoilTemperature() const { return m_nightSoilTemperature; }
  virtual void DayAirTemperature(float value) { m_dayAirTemperature = value; }
  virtual float DayAirTemperature() const { return m_dayAirTemperature; }
  virtual void NightAirTemperature(float value) { m_nightAirTemperature = value; }
  virtual float NightAirTemperature() const { return m_nightAirTemperature; }
  virtual void WaterHeaterIsOn(bool value) { m_waterHeaterIsOn = value; }
  virtual bool WaterHeaterIsOn() const { return m_waterHeaterIsOn; }
  virtual void SoilHeatingIsOn(bool value) { m_soilHeatingIsOn = value; }
  virtual bool SoilHeatingIsOn() const { return m_soilHeatingIsOn; }
  virtual void AirHeatingIsOn(bool value) { m_airHeatingIsOn = value; }
  virtual bool AirHeatingIsOn() const { return m_airHeatingIsOn; }
  virtual void WaterHeaterCostCumulative(float value) { m_waterHeaterCostCumulative = value; }
  virtual float WaterHeaterCostCumulative() const { return m_waterHeaterCostCumulative; }
  virtual void WaterHeaterDayLimitMinutes(int value) { m_waterHeaterDayLimitMinutes = value; }
  virtual int WaterHeaterDayLimitMinutes() const { return m_waterHeaterDayLimitMinutes; }
  virtual void WaterHeaterDayRuntimeMinutes(float value) { m_waterHeaterDayRuntimeMinutes = value; }
  virtual float WaterHeaterDayRuntimeMinutes() const { return m_waterHeaterDayRuntimeMinutes; }
  virtual void WaterHeaterNightLimitMinutes(int value) { m_waterHeaterNightLimitMinutes = value; }
  virtual int WaterHeaterNightLimitMinutes() const { return m_waterHeaterNightLimitMinutes; }
  virtual void WaterHeaterNightRuntimeMinutes(float value) { m_waterHeaterNightRuntimeMinutes = value; }
  virtual float WaterHeaterNightRuntimeMinutes() const { return m_waterHeaterNightRuntimeMinutes; }

private:
  void UpdatePeriod(float waterTarget, float soilTarget, float airTarget);

public:
  // getters & setters
  virtual ISystem &System() const {
    if (m_system == nullptr) {
      Log().Trace("System not set");
      throw std::runtime_error("System not set");
    }
    return *m_system;
  }
  void System(ISystem &value) { m_system = &value; }

private:
  common::Log m_log;
  ISystem *m_system;
  bool m_enabled;
  float m_dayWaterTemperature;
  float m_nightWaterTemperature;
  float m_daySoilTemperature;
  float m_nightSoilTemperature;
  float m_dayAirTemperature;
  float m_nightAirTemperature;
  bool m_waterHeaterIsOn;
  bool m_soilHeatingIsOn;
  bool m_airHeatingIsOn;
  unsigned long m_waterHeaterLastUpdate;
  float m_waterHeaterCostCumulative;
  float m_waterHeaterDayRuntimeMinutes;
  int m_waterHeaterDayLimitMinutes;
  float m_waterHeaterNightRuntimeMinutes;
  int m_waterHeaterNightLimitMinutes;
};

} // namespace greenhouse
} // namespace native
