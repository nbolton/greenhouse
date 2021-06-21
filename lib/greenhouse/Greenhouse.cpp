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

  Log().Trace("Temperature: %dC | Humidity: %d%%", t, h);

  ReportDhtValues();

  FlashLed(2);

  if (t != unknown) {
    Log().Trace("Temp is known");

    if (m_autoMode && (m_openTemp != unknown)) {
      Log().Trace("In auto mode and open temp is known");

      if (t > m_openTemp) {
        Log().Trace("Temp is above open temp");

        if (m_windowState == windowClosed) {
          Log().Trace("Window is closed, opening window");
          OpenWindow();
        }
        else {
          Log().Trace("Window already open");
        }
      }
      else {
        Log().Trace("Temp is below open temp");

        if (m_windowState == windowOpen) {
          Log().Trace("Window is open, closing window");
          CloseWindow();
        }
        else {
          Log().Trace("Window already closed");
        }
      }
    }
    else {
      Log().Trace("In manual mode or open temp unknown");
    }
  }
  else {
    Log().Trace("Temp is unknown");
  }

  return ok;
}
