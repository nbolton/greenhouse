#include "test_system.h"
#include "test_heating.h"

#include "../embedded/greenhouse/System.h"

#include <unity.h>

void process()
{
  UNITY_BEGIN();
  testSystem();
  testHeating();
  UNITY_END();
}

void setup()
{
  process();
}

void loop() {}
