#include "Greenhouse.h"

#include <stdio.h>

const int unknown = -1;

Greenhouse::Greenhouse() :
  m_dhtFailSent(false),
  m_autoMode(false),
  m_openTemp(unknown),
  m_ws(windowUnknown)
{
}

void Greenhouse::Setup()
{
}

void Greenhouse::Loop()
{
}

bool Greenhouse::Refresh()
{
  Log().Trace("Refresh");
  bool ok = true;
  
  float t = GetTemperature();
  float h = GetHumidity();
  
  if (!IsDhtReady()) {
    
    ok = false;
    const char dhtFail[] = "DHT device unavailable";
    Log().Trace(dhtFail);

    // only send once per reboot (don't spam the timeline)
    if (!m_dhtFailSent) {
      Log().ReportWarning(dhtFail);
      m_dhtFailSent = true;
    }
    
    t = unknown;
    h = unknown;
  }
  
  Log().Trace("Temperature: %dC | Humidity: %d%%", t, h);

  ReportTemperature(t);
  ReportHumidity(t);

  FlashLed(2);

  if (t != unknown) {
    Log().Trace("t known");
    if (m_autoMode && (m_openTemp != unknown)) {
      Log().Trace("auto mode & open temp known");
      if (t > m_openTemp) {
        Log().Trace("t > openTemp");
        if (m_ws == windowClosed) {
          Log().Trace("ws == windowClosed");
          OpenWindow();
        }
        else {
          Log().Trace("window already open");
        }
      }
      else {
        Log().Trace("t <= openTemp");
        if (m_ws == windowOpen) {
          Log().Trace("ws == windowOpen");
          CloseWindow();
        }
        else {
          Log().Trace("window already closed");
        }
      }
    }
    else {
      Log().Trace("manual mode or open temp unknown");
    }
  }
  else {
      Log().Trace("t unknown");
  }

  return ok;
}