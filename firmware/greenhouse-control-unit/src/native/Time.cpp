#include "Time.h"

#include "common.h"
#include <time.h>

namespace greenhouse {
namespace native {

using namespace common;

Time::Time() :
  m_dayStartHour(k_unknown),
  m_dayEndHour(k_unknown),
  m_nightToDayTransitionTime(k_unknownUL),
  m_dayToNightTransitionTime(k_unknownUL)
{
}

bool Time::IsDaytime() const
{
  return (CurrentHour() >= DayStartHour()) && (CurrentHour() < DayEndHour());
}

void Time::CheckTransition()
{
  // if time unknown, we can't do anything.
  if (!IsValid()) {
    TRACE(TRACE_DEBUG1, "Epoch unknown, can't check time transition");
    return;
  }

  bool daytime = IsDaytime();
  time_t now = EpochTime();
  time_t last;

  if (daytime) {
    last = NightToDayTransitionTime();
  }
  else {
    last = DayToNightTransitionTime();
  }

  // HACK: cast to int to print -1 (%lld doesn't seem to print -1).
  // this is a bad idea, and will break in 2038.
  TRACE_F(
    TRACE_DEBUG1,
    "Now %s, checking for %s transition, now=%d, last=%d",
    daytime ? "daytime" : "nighttime",
    daytime ? "night to day" : "day to night",
    static_cast<int>(now),
    static_cast<int>(last));

  if (last == k_unknownUL) {
    TRACE(TRACE_DEBUG1, "Last transition not known, assume it never happened");
    Transition(IsDaytime());
    return;
  }

  struct tm *ptm;

  ptm = gmtime(&now);
  int nowDay = ptm->tm_mday;
  int nowMonth = ptm->tm_mon;
  int nowYear = ptm->tm_year;
  int nowHour = ptm->tm_hour;

  TRACE_F(
    TRACE_DEBUG1,
    "Current time, hour=%d, day=%d, month=%d, year=%d",
    nowHour,
    nowDay,
    nowMonth,
    nowYear);

  ptm = gmtime(&last);
  int lastDay = ptm->tm_mday;
  int lastMonth = ptm->tm_mon;
  int lastYear = ptm->tm_year;
  int lastHour = ptm->tm_hour;

  TRACE_F(
    TRACE_DEBUG1,
    "Last transition, hour=%d, day=%d, month=%d, year=%d",
    lastHour,
    lastDay,
    lastMonth,
    lastYear);

  const int fullDay = (3600 * 24); // 24h
  if ((now - last) > fullDay) {

    TRACE(TRACE_DEBUG1, "Transition is overdue (24 hours have passed since last)");
    Transition(IsDaytime());
    return;
  }

  // if last transition didn't happen within this exact hour time frame
  if (
    (nowHour != lastHour) || (nowDay != lastDay) || (nowMonth != lastMonth) ||
    (nowYear != lastYear)) {
    TRACE(TRACE_DEBUG1, "Last transition didn't happen in current hour");

    if (DayStartHour() == nowHour) {
      TRACE(TRACE_DEBUG1, "Start of day hour; night to day transition should happen");
      Transition(IsDaytime());
      return;
    }
    else if (DayEndHour() == nowHour) {
      TRACE(TRACE_DEBUG1, "End of day hour; day to night transition should happen");
      Transition(IsDaytime());
      return;
    }
    else {
      TRACE(TRACE_DEBUG1, "Current hour is not a transition hour");
    }
  }
  else {
    TRACE(TRACE_DEBUG1, "Transition already happened in current hour");
  }

  TRACE(TRACE_DEBUG1, "No time transition happened (no transition conditions met)");
}

void Time::Transition(bool nightToDay)
{
  if (nightToDay) {
    TRACE(TRACE_DEBUG1, "Transitioning from night to day");
    NightToDayTransitionTime(EpochTime());
    OnNightToDayTransition();
  }
  else {
    TRACE(TRACE_DEBUG1, "Transitioning from day to night");
    DayToNightTransitionTime(EpochTime());
    OnDayToNightTransition();
  }
}

void Time::OnNightToDayTransition() { System().HandleNightToDayTransition(); }

void Time::OnDayToNightTransition() { System().HandleDayToNightTransition(); }

} // namespace native
} // namespace greenhouse