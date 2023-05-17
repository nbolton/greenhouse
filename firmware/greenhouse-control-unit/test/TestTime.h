#pragma once

#include "native/Time.h"

using namespace greenhouse::native;

class TestTime : public Time {

public:
  TestTime() :
    // call counters
    m_calls_OnNightToDayTransition(0),
    m_calls_OnDayToNightTransition(0),

    // mock returns
    m_mock_CurrentHour(0),
    m_mock_UptimeSeconds(0),
    m_mock_IsDaytime(false),
    m_mock_EpochTime(0),

    // mock enable
    m_mockOn_IsDaytime(false)
  {
  }

  int CurrentHour() const
  {
    TRACE_F("Mock: CurrentHour, value=%d", m_mock_CurrentHour);
    return m_mock_CurrentHour;
  }

  unsigned long UptimeSeconds() const
  {
    TRACE_F("Mock: UptimeSeconds, value=%d", static_cast<int>(m_mock_UptimeSeconds));
    return m_mock_UptimeSeconds;
  }

  bool IsDaytime() const
  {
    if (m_mockOn_IsDaytime) {
      TRACE_F("Mock: IsDaytime, value=%s", m_mock_IsDaytime ? "true" : "false");
      return m_mock_IsDaytime;
    }

    return Time::IsDaytime();
  }

  unsigned long EpochTime() const
  {
    TRACE_F("Mock: EpochTime, value=%d", static_cast<int>(m_mock_EpochTime));
    return m_mock_EpochTime;
  }

  void OnNightToDayTransition()
  {
    TRACE("Stub: OnNightToDayTransition");
    m_calls_OnNightToDayTransition++;
  }

  void OnDayToNightTransition()
  {
    TRACE("Stub: OnDayToNightTransition");
    m_calls_OnDayToNightTransition++;
  }

  // call counters (init to 0)

  int m_calls_OnNightToDayTransition;
  int m_calls_OnDayToNightTransition;

  // mock enable

  bool m_mockOn_IsDaytime;

  // mock values

  int m_mock_CurrentHour;
  unsigned long m_mock_UptimeSeconds;
  bool m_mock_IsDaytime;
  unsigned long m_mock_EpochTime;
};
