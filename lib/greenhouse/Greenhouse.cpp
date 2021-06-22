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
  float temperature = Temperature();
  float humidity = Humidity();

  if (!ok) {
    Log().Trace("DHT device unavailable");

    // only send once per reboot (don't spam the timeline)
    if (!m_dhtFailSent) {
      ReportWarning("DHT device unavailable");
      m_dhtFailSent = true;
    }

    temperature = unknown;
    humidity = unknown;
  }

  Log().Trace("Temperature: %.2fC | Humidity: %.2f%%", temperature, humidity);

  ReportDhtValues();

  FlashLed(2);

  bool windowMoved = false;

  const bool boundsKnown = (m_openStart != unknown) && (m_openFinish != unknown);
  if (m_autoMode && (temperature != unknown) && boundsKnown) {

    // TODO: https://github.com/nbolton/home-automation/issues/20
    float openStart = (float)m_openStart;
    float openFinish = (float)m_openFinish;

    Log().Trace(
      "Checking window bounds, wp=%d t=%.2f os=%.2f of=%.2f",
      WindowProgress(),
      temperature,
      openStart,
      openFinish);

    if ((temperature >= openStart) && (temperature <= openFinish)) {
      // window should be semi-open
      Log().Trace("Temperature in bounds");

      float tempWidth = openFinish - openStart;
      float progressAsTemp = temperature - openStart;
      float expectedProgress = progressAsTemp / tempWidth;

      windowMoved = ApplyWindowProgress(expectedProgress);
    }
    else if (temperature > openFinish) {
      // window should be fully open
      Log().Trace("Temperature above bounds");
      windowMoved = ApplyWindowProgress(1);
    }
    else {
      // window should be fully closed
      Log().Trace("Temperature below bounds");
      windowMoved = ApplyWindowProgress(0);
    }
  }

  // Window state is only reported back when a window opens or closes;
  // Blynk doesn't show the latest switch values that were changed while the
  // app was out of focus, so if window isn't opened or closed, report the
  // current window state back.
  if (!windowMoved) {
    ReportWindowProgress();
  }

  ReportLastRefresh();

  return ok;
}

bool Greenhouse::ApplyWindowProgress(float expectedProgress)
{
  float currentProgress = (float)WindowProgress() / 100;

  Log().Trace(
    "Applying window progress, expected=%.2f, current=%.2f", expectedProgress, currentProgress);

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
