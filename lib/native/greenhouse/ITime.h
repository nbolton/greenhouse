#pragma once

namespace native {
namespace greenhouse {

class ITime {
public:
  virtual int CurrentHour() const = 0;
  virtual bool IsDaytime() const = 0;
  virtual unsigned long UptimeSeconds() const = 0;
  virtual void CheckTimeTransition() = 0;
};

} // namespace greenhouse
} // namespace native
