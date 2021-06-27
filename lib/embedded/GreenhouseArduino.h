#pragma once

#include "Greenhouse.h"

#include "ArduinoLog.h"

#include <Arduino.h>

class GreenhouseArduino : public Greenhouse {
public:
  static GreenhouseArduino &Instance();
  static void Instance(GreenhouseArduino &ga);

  GreenhouseArduino();

  void Setup();
  void Loop();
  bool Refresh();

  void ReportInfo(const char *m, ...);
  void ReportWarning(const char *m, ...);
  void ReportCritical(const char *m, ...);

  const ArduinoLog &Log() const { return m_log; }
  void TraceFlash(const __FlashStringHelper *f) const { m_log.TraceFlash(f); }

  void HandleAutoMode(bool autoMode);
  void HandleOpenStart(float openStart);
  void HandleOpenFinish(float openFinish);
  void HandleRefreshRate(int refreshRate);
  void HandleWindowProgress(int windowProgress);
  void HandleReset(int reset);
  void HandleRefresh(int refresh);
  void HandleFakeSoilTemperature(float fakeTemperature);
  void HandleTestMode(int fakeMode);
  void HandleOpenDayMinimum(int openDayMinimum);
  void HandleLastWrite();
  void HandleSystemStarted();

protected:
  void FlashLed(LedFlashTimes times);
  bool ReadSensors();
  float InsideTemperature() const { return m_insideTemperature; }
  float InsideHumidity() const { reutrn m_insideHumidity; }
  float OutsideTemperature() const { return m_outsideTemperature; }
  float OutsideHumidity() const { return m_outsideHumidity; }
  float SoilTemperature() const;
  float SoilMoisture() const;
  void Reset();
  int CurrentHour() const;

  virtual void OpenWindow(float delta);
  virtual void CloseWindow(float delta);
  virtual void SystemDigitalWrite(int pin, int val);
  virtual void SystemDelay(unsigned long ms);

  void ReportSensorValues();
  void ReportWindowProgress();
  void ReportSystemInfo();

private:
  ArduinoLog m_log;
  float m_insideTemperature;
  float m_insideHumidity;
  float m_outsideTemperature;
  float m_outsideHumidity;
  float m_soilTemperature;
  int m_timerId;
  int m_led;
  float m_fakeSoilTemperature;
  float m_fakeSoilMoisture;
  bool m_refreshBusy;
  bool m_lastWriteDone;
  float m_soilMoisture;
};
