#include "Greenhouse.h"

#include <stdio.h>

const int unknown = -1;

Greenhouse::Greenhouse() :
  m_dhtFailSent(false),
  m_autoMode(false),
  m_openTemp(unknown),
  m_windowState(windowUnknown)
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
    const char dhtFail[] = "DHT device unavailable";
    Log().Trace(dhtFail);

    // only send once per reboot (don't spam the timeline)
    if (!m_dhtFailSent) {
      ReportWarning(dhtFail);
      m_dhtFailSent = true;
    }

    t = unknown;
    h = unknown;
  }

  Log().Trace("Temperature: %.2fC | Humidity: %.2f%%", t, h);

  ReportDhtValues();

  FlashLed(2);

  if (m_autoMode && (t != unknown) && (m_openTemp != unknown)) {
    
    // TODO: https://github.com/nbolton/home-automation/issues/20
    if (t > (float)m_openTemp) {
      if (m_windowState != windowOpen) {
        OpenWindow();
      }
    }
    else {
      if (m_windowState != windowClosed) {
        CloseWindow();
      }
    }
  }

  return ok;
}
