#include "Greenhouse.h"
#include <unity.h>

class GreenhouseTest : public Greenhouse {
public:
  GreenhouseTest();

  bool ReadDhtSensor() { return m_mock_ReadDhtSensor; }
  float Temperature() const { return m_mock_Temperature; }
  float Humidity() const { return m_mock_Humidity; }
  void OpenWindow() { m_calls_OpenWindow++; }
  void CloseWindow() { m_calls_CloseWindow++; }

  void WindowState(::WindowState ws) { Greenhouse::WindowState(ws); }
  void AutoMode(bool am) { Greenhouse::AutoMode(am); }
  void OpenTemp(bool ot) { return Greenhouse::OpenTemp(ot); }

  bool m_mock_ReadDhtSensor;
  float m_mock_Temperature;
  float m_mock_Humidity;
  int m_calls_OpenWindow;
  int m_calls_CloseWindow;
};

const int unknown = -1;

GreenhouseTest::GreenhouseTest() :
  m_mock_ReadDhtSensor(false),
  m_mock_Temperature(unknown),
  m_mock_Humidity(unknown),
  m_calls_OpenWindow(0),
  m_calls_CloseWindow(0)
{
}

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void Test_Refresh_DhtNotReady_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = false;

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_ManualMode_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 1;
  greenhouse.AutoMode(false);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAndNoOpenTemp_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 1;
  greenhouse.AutoMode(true);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAndAboveOpenTemp_WindowOpens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 1.1;
  greenhouse.AutoMode(true);
  greenhouse.OpenTemp(1);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAndBelowOpenTemp_WindowCloses(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 0.9;
  greenhouse.AutoMode(true);
  greenhouse.OpenTemp(1);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAndAboveOpenTempAndAlreadyOpen_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 1.1;
  greenhouse.AutoMode(true);
  greenhouse.OpenTemp(1);
  greenhouse.WindowState(windowOpen);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAndBelowOpenTempAndAlreadyClosed_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 0.9;
  greenhouse.AutoMode(true);
  greenhouse.OpenTemp(1);
  greenhouse.WindowState(windowClosed);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void process()
{
  UNITY_BEGIN();
  RUN_TEST(Test_Refresh_DhtNotReady_NothingHappens);
  RUN_TEST(Test_Refresh_ManualMode_NothingHappens);
  RUN_TEST(Test_Refresh_AutoModeAndNoOpenTemp_NothingHappens);
  RUN_TEST(Test_Refresh_AutoModeAndAboveOpenTemp_WindowOpens);
  RUN_TEST(Test_Refresh_AutoModeAndBelowOpenTemp_WindowCloses);
  RUN_TEST(Test_Refresh_AutoModeAndAboveOpenTempAndAlreadyOpen_NothingHappens);
  RUN_TEST(Test_Refresh_AutoModeAndBelowOpenTempAndAlreadyClosed_NothingHappens);
  UNITY_END();
}

#ifdef ARDUINO

#include <Arduino.h>
void setup()
{
  // NOTE!!! Wait for >2 secs
  // if board doesn't support software reset via Serial.DTR/RTS
  delay(2000);

  process();
}

void loop()
{
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(500);
}

#else

int main(int argc, char **argv)
{
  process();
  return 0;
}

#endif
