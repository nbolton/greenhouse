#pragma once

#include "Log.h"

class Greenhouse {
public:
  Greenhouse();

  virtual void Setup();
  virtual void Loop();
  virtual bool Refresh();

  virtual void ReportInfo(const char *m, ...) const {}
  virtual void ReportWarning(const char *m, ...) const {}
  virtual void ReportCritical(const char *m, ...) const {}

  virtual const ::Log &Log() const { return m_log; }

protected:
  virtual bool ReadDhtSensor() { return false; }
  virtual float Temperature() const { return -1; }
  virtual float Humidity() const { return -1; }
  virtual void ReportDhtValues() {}
  virtual void FlashLed(int times) {}
  virtual void OpenWindow(float delta) {}
  virtual void CloseWindow(float delta) {}
  virtual void ReportWindowProgress() {}

  virtual void WindowProgress(int windowProgress) { m_windowProgress = windowProgress; }
  virtual int WindowProgress() const { return m_windowProgress; }
  virtual void AutoMode(bool am) { m_autoMode = am; }
  virtual bool AutoMode() const { return m_autoMode; }
  virtual void OpenStart(bool openStart) { m_openStart = openStart; }
  virtual int OpenStart() const { return m_openStart; }
  virtual void OpenFinish(bool openFinish) { m_openFinish = openFinish; }
  virtual int OpenFinish() const { return m_openFinish; }

  virtual bool ApplyWindowProgress(float expectedProgress);

private:
  ::Log m_log;
  bool m_dhtFailSent;
  bool m_autoMode;
  int m_openStart;
  int m_openFinish;
  int m_windowProgress;
};
