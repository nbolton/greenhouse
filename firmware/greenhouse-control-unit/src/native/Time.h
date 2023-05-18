#pragma once

#include "ISystem.h"
#include "ITime.h"
#include "common.h"

#include <stdexcept>
#include <trace.h>

namespace greenhouse {
namespace native {

using namespace common;

class Time : public ITime {
public:
  Time();
  virtual void CheckTransition();
  virtual bool IsValid() const { return false; }
  virtual unsigned long EpochTime() const { return k_unknown; }
  unsigned long UptimeSeconds() const { return k_unknown; }
  virtual int CurrentHour() const { return k_unknown; }
  virtual bool IsDaytime() const;
  virtual void DayStartHour(int value) { m_dayStartHour = value; }
  virtual int DayStartHour() const { return m_dayStartHour; }
  virtual void DayEndHour(int value) { m_dayEndHour = value; }
  virtual int DayEndHour() const { return m_dayEndHour; }
  virtual void NightToDayTransitionTime(unsigned long value) { m_nightToDayTransitionTime = value; }
  virtual unsigned long NightToDayTransitionTime() const { return m_nightToDayTransitionTime; }
  virtual void DayToNightTransitionTime(unsigned long value) { m_dayToNightTransitionTime = value; }
  virtual unsigned long DayToNightTransitionTime() const { return m_dayToNightTransitionTime; }

public:
  // getters & setters
  virtual ISystem &System() const
  {
    if (m_system == nullptr) {
      TRACE(TRACE_DEBUG1, "System not set");
      throw std::runtime_error("System not set");
    }
    return *m_system;
  }
  void System(ISystem &value) { m_system = &value; }

protected:
  virtual void Transition(bool nightToDay);
  virtual void OnNightToDayTransition();
  virtual void OnDayToNightTransition();

private:
  ISystem *m_system;
  int m_dayStartHour;
  int m_dayEndHour;
  unsigned long m_nightToDayTransitionTime;
  unsigned long m_dayToNightTransitionTime;
};

} // namespace native
} // namespace greenhouse
