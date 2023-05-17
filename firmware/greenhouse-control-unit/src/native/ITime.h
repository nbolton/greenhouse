#pragma once

namespace greenhouse {
namespace native {

class ITime {
public:
  virtual int CurrentHour() const = 0;
  virtual bool IsDaytime() const = 0;
  virtual unsigned long UptimeSeconds() const = 0;
  virtual void CheckTransition() = 0;
};

} // namespace native
} // namespace greenhouse
