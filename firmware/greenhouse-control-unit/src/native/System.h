#pragma once

#include "Heating.h"
#include "ISystem.h"
#include "Time.h"
#include "common.h"

#include <queue>
#include <string>
#include <trace.h>

namespace greenhouse {
namespace native {

using namespace common;

class System : public ISystem {

public:
  System();

  virtual greenhouse::native::Time &Time() = 0;
  virtual void Setup();
  virtual void Loop();
  virtual void Refresh(bool first);
  virtual void ReportInfo(const char *m, ...) {}
  virtual void ReportWarning(const char *m, ...) {}
  virtual void ReportCritical(const char *m, ...) {}
  virtual void WindowFullClose();

public:
  void UpdateWindowOpenPercent();

protected:
  virtual void ReportSensorValues() {}
  virtual void ReportWindowOpenPercent() {}
  virtual void ReportSystemInfo() {}
  virtual void ReportWarnings() {}
  virtual void ReportWeather() {}
  virtual void ReportWaterHeaterInfo() {}
  virtual bool ReadSensors(int &failures) { return false; }
  virtual void OpenWindow(float delta);
  virtual void CloseWindow(float delta);
  virtual void ApplyWindowOpenPercent();
  virtual void ChangeWindowOpenPercentActual(float delta);
  virtual void RunWindowActuator(bool extend, float delta) {}
  virtual void Delay(unsigned long ms, const char *reason) {}
  virtual bool UpdateWeatherForecast() { return false; }
  virtual void HandleNightToDayTransition();
  virtual void HandleDayToNightTransition();

public:
  // getters & setters
  virtual greenhouse::native::Heating &Heating() = 0;
  virtual float InsideAirTemperature() const { return k_unknown; }
  virtual float InsideAirHumidity() const { return k_unknown; }
  virtual float OutsideAirTemperature() const { return k_unknown; }
  virtual float OutsideAirHumidity() const { return k_unknown; }
  virtual float SoilTemperature() const { return k_unknown; }
  virtual float WaterTemperature() const { return k_unknown; }
  virtual void AutoMode(bool value) { m_autoMode = value; }
  virtual bool AutoMode() const { return m_autoMode; }
  virtual void OpenStart(float value) { m_openStart = value; }
  virtual float OpenStart() const { return m_openStart; }
  virtual void OpenFinish(float value) { m_openFinish = value; }
  virtual float OpenFinish() const { return m_openFinish; }
  virtual void TestMode(bool value) { m_testMode = value; }
  virtual bool TestMode() const { return m_testMode; }
  virtual void WindowActuatorRuntimeSec(float value) { m_windowActuatorRuntimeSec = value; }
  virtual float WindowActuatorRuntimeSec() const { return m_windowActuatorRuntimeSec; }
  virtual void WeatherCode(int value) { m_weatherCode = value; }
  virtual int WeatherCode() const;
  virtual void WeatherInfo(std::string value) { m_weatherInfo = value; }
  virtual const std::string &WeatherInfo() const { return m_weatherInfo; }
  virtual void SystemStarted(bool value) { m_systemStarted = value; }
  virtual bool SystemStarted() const { return m_systemStarted; }
  virtual void WindowAdjustPositions(int value) { m_windowAdjustPositions = value; }
  virtual int WindowAdjustPositions() { return m_windowAdjustPositions; }
  virtual void WindowAdjustTimeframeSec(int value) { m_windowAdjustTimeframeSec = value; }
  virtual int WindowAdjustTimeframeSec() { return m_windowAdjustTimeframeSec; }
  virtual void WindowOpenPercent(int value);
  virtual int WindowOpenPercentExpected() const { return m_windowOpenPercentExpected; }
  virtual int WindowOpenPercentActual() const { return m_windowOpenPercentActual; }
  virtual void FakeWeatherCode(int value) { m_fakeWeatherCode = value; }
  virtual int FakeWeatherCode() const { return m_fakeWeatherCode; }

private:
  bool IsRaining() const;

private:
  bool m_sensorWarningSent;
  bool m_autoMode;
  float m_openStart;
  float m_openFinish;
  bool m_applyWindowOpenPercent;
  int m_windowOpenPercentExpected;
  int m_windowOpenPercentActual;
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
  int m_windowAdjustPositions;
  int m_windowAdjustTimeframeSec;
  unsigned long m_windowAdjustLast;
  unsigned long m_windowActuatorStopTime;
  int m_fakeWeatherCode;
};

} // namespace native
} // namespace greenhouse
