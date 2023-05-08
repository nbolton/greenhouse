#pragma once

#define HALT()                                                                                     \
  TRACE(TRACE_INFO, "halt");                                                                       \
  while (true)                                                                                     \
  delay(1)

float mapFloat(float v, float inMin, float inMax, float outMin, float outMax);
