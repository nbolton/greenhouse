#include "Time.h"

#include "common.h"
#include <time.h>

namespace native {
namespace greenhouse {

using namespace common;

Time::Time() :
  m_nightToDayTransitionTime(k_unknownUL),
  m_dayToNightTransitionTime(k_unknownUL),
  m_dayStartHour(k_unknown),
  m_dayEndHour(k_unknown)
{
}

bool Time::IsDaytime() const
{
  return (CurrentHour() >= DayStartHour()) && (CurrentHour() < DayEndHour());
}

void Time::CheckTimeTransition()
{
  // if time unknown, we can't do anything.
  if (EpochTime() == k_unknownUL) {
    Log().Trace("Epoch unknown, can't check time transition");
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
  Log().Trace(
    "Now %s, checking for %s transition, now=%d, last=%d",
    daytime ? "daytime" : "nighttime",
    daytime ? "night to day" : "night to day",
    static_cast<int>(now),
    static_cast<int>(last));

  if (last == k_unknownUL) {
    Log().Trace("Last transition not known, assume it never happened");
    TransitionTimeNow();
    return;
  }

  struct tm *ptm;

  ptm = gmtime(&now);
  int nowDay = ptm->tm_mday;
  int nowMonth = ptm->tm_mon;
  int nowYear = ptm->tm_year;
  int nowHour = ptm->tm_hour;

  Log().Trace(
    "Current time, hour=%d, day=%d, month=%d, year=%d", nowHour, nowDay, nowMonth, nowYear);

  ptm = gmtime(&last);
  int lastDay = ptm->tm_mday;
  int lastMonth = ptm->tm_mon;
  int lastYear = ptm->tm_year;
  int lastHour = ptm->tm_hour;

  Log().Trace(
    "Last transition, hour=%d, day=%d, month=%d, year=%d", lastHour, lastDay, lastMonth, lastYear);

  const int fullDay = (3600 * 24); // 24h
  if ((now - last) > fullDay) {

    Log().Trace("Transition is overdue (24 hours have passed since last)");
    TransitionTimeNow();
    return;
  }

  // if last transition didn't happen within this exact hour time frame
  if (
    (nowHour != lastHour) && (nowDay != lastDay) && (nowMonth != lastMonth) &&
    (nowYear != lastYear)) {

    if (DayStartHour() == nowHour) {
      Log().Trace("Start of day hour; night to day transition should happen");
      TransitionTimeNow();
      return;
    }

    if (DayEndHour() == nowHour) {
      Log().Trace("End of day hour; day to night transition should happen");
      TransitionTimeNow();
      return;
    }
  }

  Log().Trace("No time transition transition happened");
}

void Time::TransitionTimeNow()
{
  if (IsDaytime()) {
    Log().Trace("Transitioning from night to day");
    NightToDayTransitionTime(EpochTime());
    System().HandleNightToDayTransition();
  }
  else {
    Log().Trace("Transitioning from day to night");
    DayToNightTransitionTime(EpochTime());
    System().HandleDayToNightTransition();
  }
}

} // namespace greenhouse
} // namespace native
