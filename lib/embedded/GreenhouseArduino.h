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

  void ReportInfo(const char *m, ...) const;
  void ReportWarning(const char *m, ...) const;
  void ReportCritical(const char *m, ...) const;

  const ArduinoLog &Log() const { return m_log; }
  void TraceFlash(const __FlashStringHelper *f) const { m_log.TraceFlash(f); }

  void HandleAutoMode(bool autoMode);
  void HandleOpenStart(bool openStart);
  void HandleOpenFinish(bool openFinish);
  void HandleRefreshRate(int refreshRate);
  void HandleWindowOpen(int openWindow);
  void HandleReset(int reset);
  void HandleRefresh(int refresh);

protected:
  bool ReadDhtSensor();
  float Temperature() const;
  float Humidity() const;
  void ReportDhtValues();
  void FlashLed(int times);
  void OpenWindow();
  void CloseWindow();
  void Reboot();
  void ReportWindowState();

private:
  ArduinoLog m_log;
  float m_temperature;
  float m_humidity;
  int m_timerId;
  int m_led;
};
