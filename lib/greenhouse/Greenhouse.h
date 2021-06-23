#pragma once

#include "Log.h"

enum LedFlashTimes { k_ledRefresh = 1, k_ledSend = 2, k_ledRecieve = 4 };

class Greenhouse {
public:
  Greenhouse();

  virtual void Setup();
  virtual void Loop();
  virtual bool Refresh();

  virtual void ReportInfo(const char *m, ...) {}
  virtual void ReportWarning(const char *m, ...) {}
  virtual void ReportCritical(const char *m, ...) {}

  virtual const ::Log &Log() const { return m_log; }

protected:
  virtual void FlashLed(LedFlashTimes times) {}
  virtual bool ReadDhtSensor() { return false; }
  virtual float Temperature() const { return -1; }
  virtual float Humidity() const { return -1; }
  virtual void OpenWindow(float delta);
  virtual void CloseWindow(float delta);

  virtual void ReportDhtValues() {}
  virtual void ReportWindowProgress() {}
  virtual void ReportSystemInfo() {}

  virtual void WindowProgress(int windowProgress) { m_windowProgress = windowProgress; }
  virtual int WindowProgress() const { return m_windowProgress; }
  virtual void AutoMode(bool am) { m_autoMode = am; }
  virtual bool AutoMode() const { return m_autoMode; }
  virtual void OpenStart(float openStart) { m_openStart = openStart; }
  virtual float OpenStart() const { return m_openStart; }
  virtual void OpenFinish(float openFinish) { m_openFinish = openFinish; }
  virtual float OpenFinish() const { return m_openFinish; }
  virtual void TestMode(bool testMode) { m_testMode = testMode; }
  virtual bool TestMode() const { return m_testMode; }

  virtual bool ApplyWindowProgress(float expectedProgress);
  virtual void AddWindowProgressDelta(float delta);

private:
  ::Log m_log;
  bool m_dhtFailSent;
  bool m_autoMode;
  float m_openStart;
  float m_openFinish;
  int m_windowProgress;
  bool m_testMode;
};
