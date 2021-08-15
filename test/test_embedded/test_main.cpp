#include "GreenhouseArduino.h"

#include "test_common.h"

#include <Arduino.h>
#include <unity.h>

class GreenhouseArduinoTest : public GreenhouseArduino {
};

GreenhouseArduinoTest greenhouse;

void process()
{
  UNITY_BEGIN();
  testCommon();
  UNITY_END();
}

void setup()
{
  GreenhouseArduinoTest::Instance(greenhouse);
  process();
}

void loop() {}
