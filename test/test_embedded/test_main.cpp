#include "test.h"

#include "../embedded/greenhouse/System.h"

#include <unity.h>

void process()
{
  UNITY_BEGIN();
  testSystem();
  testHeating();
  testTime();
  UNITY_END();
}

void setup()
{
  process();
}

void loop() {}
