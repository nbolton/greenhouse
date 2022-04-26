#include "Time.h"
#include "common.h"

#include <NTPClient.h>
#include <WiFiUdp.h>

namespace embedded {
namespace greenhouse {

using namespace common;

static WiFiUDP s_ntpUdp;
static NTPClient s_timeClient(s_ntpUdp);

Time::Time() : m_timeClientOk(false) {}

void Time::Setup() { s_timeClient.begin(); }

void Time::Refresh()
{
  Log().Trace(F("Time refresh"));
  
  const int retryDelay = 1000;
  const int retryLimit = 5;
  int retryCount = 1;

  do {
    m_timeClientOk = s_timeClient.update();
    if (!m_timeClientOk) {
      Log().Trace(F("Time update failed (attempt %d)"), retryCount);
      System().Delay(retryDelay, "time update");
    }
  } while (!m_timeClientOk && retryCount++ < retryLimit);
}

int Time::CurrentHour() const
{
  if (!m_timeClientOk) {
    return k_unknown;
  }
  return s_timeClient.getHours();
}

unsigned long Time::EpochTime() const
{
  if (!m_timeClientOk) {
    return k_unknown;
  }
  return s_timeClient.getEpochTime();
}

unsigned long Time::UptimeSeconds() const { return millis() / 1000; }

String Time::FormattedCurrentTime()
{
  return s_timeClient.getFormattedTime() + " UTC";
}

String Time::FormattedUptime()
{
  int seconds, minutes, hours, days;
  long uptime = UptimeSeconds();
  minutes = uptime / 60;
  seconds = uptime % 60;
  hours = minutes / 60;
  minutes = minutes % 60;
  days = hours / 24;
  hours = hours % 24;
  char uptimeBuffer[50];
  sprintf(uptimeBuffer, "%dd %dh %dm %ds", days, hours, minutes, seconds);
  return uptimeBuffer;
}

} // namespace greenhouse
} // namespace embedded
