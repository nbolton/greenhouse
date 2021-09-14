#pragma once

namespace embedded {
namespace greenhouse {

class ISystem {
public:
  virtual void SetSwitch(int i, bool on) = 0;
};

} // namespace greenhouse
} // namespace embedded
