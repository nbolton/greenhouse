#pragma once

#include "Log.h"

const int k_unknown = -1;

enum LedFlashTimes {
  k_ledRefresh = 1,
  k_ledSend = 2,
  k_ledRecieve = 3,
  k_ledStarted = 4,
  k_ledReset = 5
};

class Greenhouse {
public:
  Greenhouse();

  virtual void Setup();
  virtual void Loop();
  virtual bool Refresh();

  virtual const ::Log &Log() const { return m_log; }

protected:
  virtual void ReportInfo(const char *m, ...) {}
  virtual void ReportWarning(const char *m, ...) {}
  virtual void ReportCritical(const char *m, ...) {}
  virtual void ReportSensorValues() {}
  virtual void ReportWindowProgress() {}
  virtual void ReportSystemInfo() {}
  virtual void ReportWarnings() {}

  virtual void FlashLed(LedFlashTimes times) {}
  virtual bool ReadSensors() { return false; }
  virtual void OpenWindow(float delta);
  virtual void CloseWindow(float delta);
  virtual float CalculateMoisture(int analogValue) const;
  virtual bool ApplyWindowProgress(float expectedProgress);
  virtual void AddWindowProgressDelta(float delta);

  // getters & setters
  virtual int CurrentHour() const { return k_unknown; }
  virtual float InsideTemperature() const { return k_unknown; }
  virtual float InsideHumidity() const { return k_unknown; }
  virtual float OutsideTemperature() const { return k_unknown; }
  virtual float OutsideHumidity() const { return k_unknown; }
  virtual float SoilTemperature() const { return k_unknown; }
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

private:
  ::Log m_log;
  bool m_sensorWarningSent;
  bool m_autoMode;
  float m_openStart;
  float m_openFinish;
  int m_windowProgress;
  bool m_testMode;
  int m_openDayMinimum;
  float m_insideHumidityWarning;
  float m_soilMostureWarning;
};
