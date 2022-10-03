#include "common.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif // ARDUINO

namespace common {

// float version of Arduino map()
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / ((in_max - in_min) + out_min);
}

void halt()
{
  while (true) {
#ifdef ARDUINO
    // prevent WDT reset
    yield();
#endif // ARDUINO
  }
}

}
