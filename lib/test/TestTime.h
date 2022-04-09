#pragma once

#include "../native/greenhouse/Time.h"

using namespace native::greenhouse;

class TestTime : public Time {

public:
  TestTime() :

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
    Log().Trace("Mock: CurrentHour, value=%d", m_mock_CurrentHour);
    return m_mock_CurrentHour;
  }

  unsigned long UptimeSeconds() const
  {
    Log().Trace("Mock: UptimeSeconds, value=%d", static_cast<int>(m_mock_UptimeSeconds));
    return m_mock_UptimeSeconds;
  }

  bool IsDaytime() const
  {
    if (m_mockOn_IsDaytime) {
      Log().Trace("Mock: IsDaytime, value=%s", m_mock_IsDaytime ? "true" : "false");
      return m_mock_IsDaytime;
    }
    
    return Time::IsDaytime();
  }

  unsigned long EpochTime() const
  {
    Log().Trace("Mock: EpochTime, value=%d", static_cast<int>(m_mock_EpochTime));
    return m_mock_EpochTime;
  }

  // mock enable

  bool m_mockOn_IsDaytime;

  // mock values

  int m_mock_CurrentHour;
  unsigned long m_mock_UptimeSeconds;
  bool m_mock_IsDaytime;
  unsigned long m_mock_EpochTime;
};
