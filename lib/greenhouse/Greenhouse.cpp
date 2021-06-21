#include "Greenhouse.h"

#include <stdio.h>

const int unknown = -1;

Greenhouse::Greenhouse() :
  m_dhtFailSent(false),
  m_autoMode(false),
  m_openStart(unknown),
  m_openFinish(unknown),
  m_windowProgress(0)
{
}

void Greenhouse::Setup() {}

void Greenhouse::Loop() {}

bool Greenhouse::Refresh()
{
  Log().Trace("Refresh");

  bool ok = ReadDhtSensor();
  float t = Temperature();
  float h = Humidity();

  if (!ok) {
    Log().Trace("DHT device unavailable");

    // only send once per reboot (don't spam the timeline)
    if (!m_dhtFailSent) {
      ReportWarning("DHT device unavailable");
      m_dhtFailSent = true;
    }

    t = unknown;
    h = unknown;
  }

  Log().Trace("Temperature: %.2fC | Humidity: %.2f%%", t, h);

  ReportDhtValues();

  FlashLed(2);

  bool noAction = true;
  if (m_autoMode && (t != unknown) && (m_openStart != unknown) && (m_openFinish != unknown)) {

    // TODO: https://github.com/nbolton/home-automation/issues/20
    float openStart = (float)m_openStart;
    float openFinish = (float)m_openFinish;

    if ((t >= openStart) && (t <= openFinish)) {
      // window should be semi-open
      Log().Trace("window should be semi-open");

      float tempWidth = openFinish - openStart;
      float progressAsTemp = t - openStart;
      float expectedProgress = progressAsTemp / tempWidth;

      noAction = !ApplyWindowProgress(expectedProgress);
    }
    else if (t > openFinish) {
      // window should be fully open
      Log().Trace("window should be fully open");
      if (WindowProgress() < 100) {
        OpenWindow(WindowProgress() / 100);
        noAction = false;
      }
    }
    else {
      // window should be fully closed
      Log().Trace("window should be fully closed");
      if (WindowProgress() > 0) {
        CloseWindow(WindowProgress() / 100);
        noAction = false;
      }
    }
  }

  // Window state is only reported back when a window opens or closes;
  // Blynk doesn't show the latest switch values that were changed while the
  // app was out of focus, so if window isn't opened or closed, report the
  // current window state back.
  if (noAction) {
    ReportWindowProgress();
  }

  ReportLastRefresh();

  return ok;
}

bool Greenhouse::ApplyWindowProgress(float expectedProgress)
{
  float currentProgress = (float)WindowProgress() / 100;

  Log().Trace(
    "Applying window progress, expected = %f, current = %f", expectedProgress, currentProgress);

  if (expectedProgress > currentProgress) {
    OpenWindow(expectedProgress - currentProgress);
    return true;
  }
  else if (expectedProgress < currentProgress) {
    CloseWindow(currentProgress - expectedProgress);
    return true;
  }

  return false;
}
