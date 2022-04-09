#pragma once

#include "../common/Log.h"
#include "../common/common.h"
#include "Heating.h"
#include "Time.h"

#include <string>

namespace native {
namespace greenhouse {

using namespace common;

class System : public ISystem {
public:
  System();

  virtual const common::Log &Log() const { return m_log; }
  virtual native::greenhouse::Time &Time() = 0;
  virtual void Setup();
  virtual void Loop();
  virtual bool Refresh();
  virtual void ReportInfo(const char *m, ...) {}
  virtual void ReportWarning(const char *m, ...) {}
  virtual void ReportCritical(const char *m, ...) {}
  virtual void SoilCalibrateWet();
  virtual void SoilCalibrateDry();

protected:
  virtual void ReportSensorValues() {}
  virtual void ReportWindowProgress() {}
  virtual void ReportSystemInfo() {}
  virtual void ReportWarnings() {}
  virtual void ReportWeather() {}
  virtual void ReportWaterHeaterInfo() {}
  virtual void ReportMoistureCalibration() {}
  virtual void SetSwitch(int index, bool on) {}
  virtual bool ReadSensors(int &failures) { return false; }
  virtual bool ReadSoilMoistureSensor() { return false; }
  virtual void OpenWindow(float delta);
  virtual void CloseWindow(float delta);
  virtual float CalculateMoisture(float analogValue) const;
  virtual bool ApplyWindowProgress(float expectedProgress);
  virtual void AddWindowProgressDelta(float delta);
  virtual void AdjustWindow(bool open, float delta);
  virtual void RunWindowActuator(bool extend) {}
  virtual void StopActuator() {}
  virtual void Delay(unsigned long ms) {}
  virtual bool UpdateWeatherForecast() { return false; }
  virtual void HandleNightToDayTransition();
  virtual void HandleDayToNightTransition();

public:
  // getters & setters
  virtual native::greenhouse::Heating &Heating() = 0;
  virtual float InsideAirTemperature() const { return k_unknown; }
  virtual float InsideAirHumidity() const { return k_unknown; }
  virtual float OutsideAirTemperature() const { return k_unknown; }
  virtual float OutsideAirHumidity() const { return k_unknown; }
  virtual float SoilTemperature() const { return k_unknown; }
  virtual float WaterTemperature() const { return k_unknown; }
  virtual float SoilMoisture() const { return k_unknown; }
  virtual void WindowProgress(int value) { m_windowProgress = value; }
  virtual int WindowProgress() const { return m_windowProgress; }
  virtual void AutoMode(bool value) { m_autoMode = value; }
  virtual bool AutoMode() const { return m_autoMode; }
  virtual void OpenStart(float value) { m_openStart = value; }
  virtual float OpenStart() const { return m_openStart; }
  virtual void OpenFinish(float value) { m_openFinish = value; }
  virtual float OpenFinish() const { return m_openFinish; }
  virtual void TestMode(bool value) { m_testMode = value; }
  virtual bool TestMode() const { return m_testMode; }
  virtual void SoilMostureWarning(float value) { m_soilMostureWarning = value; }
  virtual float SoilMostureWarning() const { return m_soilMostureWarning; }
  virtual void WindowActuatorRuntimeSec(float value) { m_windowActuatorRuntimeSec = value; }
  virtual int WindowActuatorRuntimeSec() const { return m_windowActuatorRuntimeSec; }
  virtual void WeatherCode(int value) { m_weatherCode = value; }
  virtual int WeatherCode() const { return m_weatherCode; }
  virtual void WeatherInfo(std::string value) { m_weatherInfo = value; }
  virtual const std::string &WeatherInfo() const { return m_weatherInfo; }
  virtual void SystemStarted(bool value) { m_systemStarted = value; }
  virtual bool SystemStarted() const { return m_systemStarted; }
  virtual void SoilSensorWet(float value) { m_soilSensorWet = value; }
  virtual float SoilSensorWet() const { return m_soilSensorWet; }
  virtual void SoilSensorDry(float value) { m_soilSensorDry = value; }
  virtual float SoilSensorDry() const { return m_soilSensorDry; }
  virtual void SoilSensor(float value) { m_soilSensor = value; }
  virtual float SoilSensor() const { return m_soilSensor; }

private:
  bool IsRaining() const;

private:
  common::Log m_log;
  bool m_sensorWarningSent;
  bool m_autoMode;
  float m_openStart;
  float m_openFinish;
  int m_windowProgress;
  bool m_testMode;
  float m_soilMostureWarning;
  float m_windowActuatorRuntimeSec;
  int m_weatherCode;
  std::string m_weatherInfo;
  bool m_isRaining;
  bool m_systemStarted;
  int m_weatherErrors;
  bool m_weatherErrorReportSent;
  float m_soilSensorWet; // V, in water
  float m_soilSensorDry; // V, in air
  float m_soilSensor;    // V, latest
};

} // namespace greenhouse
} // namespace native
