#include "common.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif // ARDUINO

namespace greenhouse {

// float version of Arduino map()
float mapFloat(float v, float inMin, float inMax, float outMin, float outMax)
{
  return (v - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

} // namespace greenhouse
