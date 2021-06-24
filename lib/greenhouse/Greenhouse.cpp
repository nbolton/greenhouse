#include "Greenhouse.h"

#include <cstdlib>
#include <stdio.h>

const float windowAdjustThreshold = 0.05;
const int k_dayStartHour = 8; // 8am
const int k_dayEndHour = 20;  // 8pm

Greenhouse::Greenhouse() :
  m_dhtFailSent(false),
  m_autoMode(false),
  m_openStart(k_unknown),
  m_openFinish(k_unknown),
  m_windowProgress(k_unknown),
  m_openDayMinimum(k_unknown)
{
}

void Greenhouse::Setup() {}

void Greenhouse::Loop() {}

bool Greenhouse::Refresh()
{
  Log().Trace("Refreshing");

  bool ok = ReadDhtSensor();
  float temperature = Temperature();
  float humidity = Humidity();

  if (!ok) {
    Log().Trace("DHT device unavailable");

    // only send once per reboot (don't spam the timeline)
    if (!m_dhtFailSent) {
      ReportWarning("DHT device unavailable");
      m_dhtFailSent = true;
    }

    temperature = k_unknown;
    humidity = k_unknown;
  }

  Log().Trace("Temperature: %.2fC | Humidity: %.2f%%", temperature, humidity);

  ReportDhtValues();

  FlashLed(k_ledRefresh);

  bool windowMoved = false;

  // TODO: https://github.com/nbolton/home-automation/issues/20
  float openStart = (float)m_openStart;
  float openFinish = (float)m_openFinish;

  if (m_autoMode) {
    Log().Trace(
      "Auto mode, wp=%d%% t=%.2fC os=%.2fC of=%.2fC",
      WindowProgress(),
      temperature,
      openStart,
      openFinish);

    if ((temperature != k_unknown) && (m_openStart != k_unknown) && (m_openFinish != k_unknown)) {

      float expectedProgress = 0;

      if ((temperature > openStart) && (temperature < openFinish)) {
        // window should be semi-open
        Log().Trace("Temperature in bounds");

        float tempWidth = openFinish - openStart;
        float progressAsTemp = temperature - openStart;
        expectedProgress = progressAsTemp / tempWidth;
      }
      else if (temperature >= openFinish) {
        // window should be fully open
        Log().Trace("Temperature above bounds");
        expectedProgress = 1;
      }
      else {
        // window should be fully closed
        Log().Trace("Temperature below bounds");
      }

      windowMoved = ApplyWindowProgress(expectedProgress);
    }
  }

  // Window state is only reported back when a window opens or closes;
  // Blynk doesn't show the latest switch values that were changed while the
  // app was out of focus, so if window isn't opened or closed, report the
  // current window state back.
  if (!windowMoved) {
    ReportWindowProgress();
  }

  ReportSystemInfo();

  return ok;
}

bool Greenhouse::ApplyWindowProgress(float expectedProgress)
{
  float currentProgress = (float)WindowProgress() / 100;

  // avoid constant micro movements; only open/close window if difference is more than 5%
  bool overThreshold = std::abs(expectedProgress - currentProgress) > windowAdjustThreshold;
  bool fullValue = (expectedProgress == 0) || (expectedProgress == 1);

  if (overThreshold || fullValue) {

    Log().Trace(
      "Testing window progress, expected=%.2f, current=%.2f", expectedProgress, currentProgress);

    if (expectedProgress > currentProgress) {
      OpenWindow(expectedProgress - currentProgress);
      return true;
    }
    else if (expectedProgress < currentProgress) {

      float closeDelta = currentProgress - expectedProgress;
      int projectedProgress = WindowProgress() - (closeDelta * 100);
      bool isDayPeriod = (CurrentHour() >= k_dayStartHour) && (CurrentHour() <= k_dayEndHour);

      // during day, don't allow window to be closed less than open day minimum (if set).
      if (isDayPeriod && (OpenDayMinimum() > 0)) {
        closeDelta -= (float)OpenDayMinimum() / 100;
      }

      Log().Trace(
        "Testing close, delta=%.2f, projected=%d, hour=%d, min=%d, day=%s",
        closeDelta,
        projectedProgress,
        CurrentHour(),
        OpenDayMinimum(),
        isDayPeriod ? "Y" : "N");

      if (closeDelta > 0) {
        CloseWindow(closeDelta);
        return true;
      }
    }
  }

  Log().Trace("No window progress change");
  return false;
}

void Greenhouse::AddWindowProgressDelta(float delta)
{
  int wp = WindowProgress();
  wp += delta * 100;
  if (wp < 0) {
    wp = 0;
  }
  else if (wp > 100) {
    wp = 100;
  }
  WindowProgress(wp);
}

void Greenhouse::OpenWindow(float delta)
{
  AddWindowProgressDelta(delta);
  ReportWindowProgress();
}

void Greenhouse::CloseWindow(float delta)
{
  AddWindowProgressDelta(delta * -1);
  ReportWindowProgress();
}
