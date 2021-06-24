#include "GreenhouseArduino.h"

#include "test_common.h"

#include <Arduino.h>
#include <unity.h>

class GreenhouseArduinoTest : public GreenhouseArduino {
public:
  void OpenWindow(float delta) { GreenhouseArduino::OpenWindow(delta); }
  void CloseWindow(float delta) { GreenhouseArduino::CloseWindow(delta); }

  void SystemDigitalWrite(int pin, int val)
  {
    Log().Trace("Mock SystemDigitalWrite, pin=%d, val=%d", pin, val);
    arg_SystemDigitalWrite_val[calls_SystemDigitalWrite] = val;
    arg_SystemDigitalWrite_pin[calls_SystemDigitalWrite] = pin;
    calls_SystemDigitalWrite++;
  }

  void SystemDelay(unsigned long ms)
  {
    Log().Trace("Mock SystemDelay, ms=%d", ms);
    arg_SystemDelay_ms[calls_SystemDelay] = ms;
    calls_SystemDelay++;
  }

  void ResetMock()
  {
    calls_SystemDigitalWrite = 0;
    calls_SystemDelay = 0;
    for (int i = 0; i < 10; i++) {
      arg_SystemDigitalWrite_pin[i] = k_unknown;
      arg_SystemDigitalWrite_val[i] = k_unknown;
      arg_SystemDelay_ms[i] = k_unknown;
    }
  }

  int arg_SystemDigitalWrite_pin[10];
  int arg_SystemDigitalWrite_val[10];
  int arg_SystemDelay_ms[10];

  int calls_SystemDigitalWrite;
  int calls_SystemDelay;
};

GreenhouseArduinoTest greenhouse;

void test_OpenWindow(void)
{
  greenhouse.ResetMock();
  greenhouse.OpenWindow(0.1);

  TEST_ASSERT_EQUAL_INT(1, greenhouse.calls_SystemDelay);
  TEST_ASSERT_EQUAL_INT(4, greenhouse.calls_SystemDigitalWrite);

  TEST_ASSERT_EQUAL_INT(1200, greenhouse.arg_SystemDelay_ms[0]);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.arg_SystemDigitalWrite_pin[0]);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.arg_SystemDigitalWrite_val[0]);
  TEST_ASSERT_EQUAL_INT(12, greenhouse.arg_SystemDigitalWrite_pin[1]);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.arg_SystemDigitalWrite_val[1]);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.arg_SystemDigitalWrite_pin[2]);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.arg_SystemDigitalWrite_val[2]);
  TEST_ASSERT_EQUAL_INT(12, greenhouse.arg_SystemDigitalWrite_pin[3]);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.arg_SystemDigitalWrite_val[3]);
}

void test_CloseWindow(void)
{
  greenhouse.ResetMock();
  greenhouse.CloseWindow(0.2);

  TEST_ASSERT_EQUAL_INT(1, greenhouse.calls_SystemDelay);
  TEST_ASSERT_EQUAL_INT(4, greenhouse.calls_SystemDigitalWrite);

  TEST_ASSERT_EQUAL_INT(2400, greenhouse.arg_SystemDelay_ms[0]);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.arg_SystemDigitalWrite_pin[0]);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.arg_SystemDigitalWrite_val[0]);
  TEST_ASSERT_EQUAL_INT(12, greenhouse.arg_SystemDigitalWrite_pin[1]);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.arg_SystemDigitalWrite_val[1]);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.arg_SystemDigitalWrite_pin[2]);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.arg_SystemDigitalWrite_val[2]);
  TEST_ASSERT_EQUAL_INT(12, greenhouse.arg_SystemDigitalWrite_pin[3]);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.arg_SystemDigitalWrite_val[3]);
}

void process()
{
  UNITY_BEGIN();
  testCommon();
  RUN_TEST(test_OpenWindow);
  RUN_TEST(test_CloseWindow);
  UNITY_END();
}

void setup()
{
  GreenhouseArduinoTest::Instance(greenhouse);
  process();
}

void loop() {}
