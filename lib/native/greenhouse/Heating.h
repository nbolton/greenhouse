#pragma once

#include "ISystem.h"
#include <stdexcept>
#include "Log.h"

namespace native {
namespace greenhouse {

class Heating {

public:
  Heating();
  void Update();
  const common::Log &Log() const { return m_log; }
  void HandleDayNightTransition();

protected:
  virtual bool SwitchWaterHeater(bool on);
  virtual bool SwitchSoilHeating(bool on);
  virtual bool SwitchAirHeating(bool on);

public:
  // getters & setters
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
  virtual void WaterHeaterLimitMinutes(int value) { m_waterHeaterLimitMinutes = value; }
  virtual int WaterHeaterLimitMinutes() const { return m_waterHeaterLimitMinutes; }
  virtual void WaterHeaterRuntimeMinutes(float value) { m_waterHeaterRuntimeMinutes = value; }
  virtual float WaterHeaterRuntimeMinutes() const { return m_waterHeaterRuntimeMinutes; }
  virtual void WaterHeaterCostCumulative(float value) { m_waterHeaterCostCumulative = value; }
  virtual float WaterHeaterCostCumulative() const { return m_waterHeaterCostCumulative; }

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
  float m_dayWaterTemperature;
  float m_nightWaterTemperature;
  float m_daySoilTemperature;
  float m_nightSoilTemperature;
  float m_dayAirTemperature;
  float m_nightAirTemperature;
  bool m_waterHeaterIsOn;
  bool m_soilHeatingIsOn;
  bool m_airHeatingIsOn;
  float m_waterHeaterRuntimeMinutes;
  int m_waterHeaterLimitMinutes;
  unsigned long m_waterHeaterLastUpdate;
  float m_waterHeaterCostCumulative;
};

} // namespace greenhouse
} // namespace native
