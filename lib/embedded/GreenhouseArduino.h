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
  void HandleFakeTemperature(float fakeTemperature);
  void HandleTestMode(int fakeMode);
  void HandleOpenDayMinimum(int openDayMinimum);

protected:
  void FlashLed(LedFlashTimes times);
  bool ReadDhtSensor();
  float Temperature() const;
  float Humidity() const;
  void Reset();
  int CurrentHour() const;

  virtual void OpenWindow(float delta);
  virtual void CloseWindow(float delta);
  virtual void SystemDigitalWrite(int pin, int val);
  virtual void SystemDelay(unsigned long ms);

  void ReportDhtValues();
  void ReportWindowProgress();
  void ReportSystemInfo();

private:
  ArduinoLog m_log;
  float m_temperature;
  float m_humidity;
  int m_timerId;
  int m_led;
  float m_fakeTemperature;
};
