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

  void ReportInfo(const char *m, ...) const;
  void ReportWarning(const char *m, ...) const;
  void ReportCritical(const char *m, ...) const;

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

protected:
  bool ReadDhtSensor();
  float Temperature() const;
  float Humidity() const;
  void ReportDhtValues();
  void FlashLed(int times);
  void OpenWindow(float delta);
  void CloseWindow(float delta);
  void Reset();
  void ReportWindowProgress();
  void ReportLastRefresh();

private:
  ArduinoLog m_log;
  float m_temperature;
  float m_humidity;
  int m_timerId;
  int m_led;

public:
  float m_fakeTemperature;
};
