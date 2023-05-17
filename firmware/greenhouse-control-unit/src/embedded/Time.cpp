#include "Time.h"
#include "common.h"

#include <NTPClient.h>
#include <WiFiUdp.h>

namespace greenhouse {
namespace embedded {

const int retryDelay = 1000;
const int k_maxSetupRetry = 5;
const int k_maxUpdateRetry = 5;

using namespace common;

static WiFiUDP s_ntpUdp;
static NTPClient s_timeClient(s_ntpUdp);

Time::Time() : m_timeClientOk(false), m_success(0), m_errors(0) {}

void Time::Setup() { s_timeClient.begin(); }

void Time::Refresh()
{
  TRACE("Doing time update");
  m_timeClientOk = s_timeClient.update();

  if (!m_timeClientOk) {
    TRACE("Error: Time update failed");
    m_errors++;
  }
  else {
    m_success++;
  }

  TRACE_F(
    "Time update stats, last=%s, success=%d, errors=%d",
    m_timeClientOk ? "ok" : "fail",
    m_success,
    m_errors);

  TRACE_F("Time is: %s", FormattedCurrentTime().c_str());
}

unsigned long Time::UptimeSeconds() const { return millis() / 1000; }

int Time::CurrentHour() const
{
  if (!IsValid()) {
    return k_unknown;
  }
  return s_timeClient.getHours();
}

unsigned long Time::EpochTime() const
{
  if (!IsValid()) {
    return k_unknown;
  }
  return s_timeClient.getEpochTime();
}

String Time::FormattedCurrentTime()
{
  if (!IsValid()) {
    return "Unknown";
  }
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

} // namespace embedded
} // namespace greenhouse
