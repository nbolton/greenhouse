#pragma once

#include "../common/Log.h"
#include "../common/common.h"
#include "Heating.h"

#include <string>

namespace native {
namespace greenhouse {

using namespace common;

class System : public ISystem {
public:
  System();

  virtual void Setup();
  virtual void Loop();
  virtual bool Refresh();
  virtual void ReportInfo(const char *m, ...) {}
  virtual void ReportWarning(const char *m, ...) {}
  virtual void ReportCritical(const char *m, ...) {}
  virtual const common::Log &Log() const { return m_log; }

protected:
  virtual void ReportSensorValues() {}
  virtual void ReportWindowProgress() {}
  virtual void ReportSystemInfo() {}
  virtual void ReportWarnings() {}
  virtual void ReportWeather() {}
  virtual void ReportWaterHeatingInfo() {}
  virtual void SetSwitch(int index, bool on) {}

  virtual bool ReadSensors(int &failures) { return false; }
  virtual void OpenWindow(float delta);
  virtual void CloseWindow(float delta);
  virtual float CalculateMoisture(float analogValue) const;
  virtual bool ApplyWindowProgress(float expectedProgress);
  virtual void AddWindowProgressDelta(float delta);
  virtual void AdjustWindow(bool open, float delta);
  virtual void RunWindowActuator(bool forward) {}
  virtual void StopActuator() {}
  virtual void SetWindowActuatorSpeed(int speed) {}
  virtual void SystemDelay(unsigned long ms) {}
  virtual bool UpdateWeatherForecast() { return false; }
  virtual void HandleNightDayTransition();

public:
  // getters & setters
  virtual native::greenhouse::Heating &Heating() = 0;
  virtual int CurrentHour() const { return k_unknown; }
  virtual unsigned long UptimeSeconds() const { return k_unknown; }
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
  virtual void OpenDayMinimum(int value) { m_openDayMinimum = value; }
  virtual int OpenDayMinimum() const { return m_openDayMinimum; }
  virtual void InsideHumidityWarning(float value) { m_insideHumidityWarning = value; }
  virtual float InsideHumidityWarning() const { return m_insideHumidityWarning; }
  virtual void SoilMostureWarning(float value) { m_soilMostureWarning = value; }
  virtual float SoilMostureWarning() const { return m_soilMostureWarning; }
  virtual void DayStartHour(int value) { m_dayStartHour = value; }
  virtual int DayStartHour() const { return m_dayStartHour; }
  virtual void DayEndHour(int value) { m_dayEndHour = value; }
  virtual int DayEndHour() const { return m_dayEndHour; }
  virtual void WindowActuatorSpeedPercent(int value) { m_windowActuatorSpeedPercent = value; }
  virtual int WindowActuatorSpeedPercent() const { return m_windowActuatorSpeedPercent; }
  virtual void WindowActuatorRuntimeSec(float value) { m_windowActuatorRuntimeSec = value; }
  virtual int WindowActuatorRuntimeSec() const { return m_windowActuatorRuntimeSec; }
  virtual void WeatherCode(int value) { m_weatherCode = value; }
  virtual int WeatherCode() const { return m_weatherCode; }
  virtual void WeatherInfo(std::string value) { m_weatherInfo = value; }
  virtual const std::string &WeatherInfo() const { return m_weatherInfo; }
  virtual void SystemStarted(bool value) { m_systemStarted = value; }
  virtual bool SystemStarted() const { return m_systemStarted; }

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
  int m_openDayMinimum;
  float m_insideHumidityWarning;
  float m_soilMostureWarning;
  int m_dayStartHour;
  int m_dayEndHour;
  int m_windowActuatorSpeedPercent;
  float m_windowActuatorRuntimeSec;
  int m_weatherCode;
  std::string m_weatherInfo;
  bool m_isRaining;
  bool m_systemStarted;
  int m_weatherErrors;
  bool m_weatherErrorReportSent;
};

} // namespace greenhouse
} // namespace native