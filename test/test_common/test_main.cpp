#include "Greenhouse.h"
#include <unity.h>

const int unknown = -1;

class GreenhouseTest : public Greenhouse {
public:
  GreenhouseTest() :
    m_mock_ReadDhtSensor(false),
    m_mock_Temperature(unknown),
    m_mock_Humidity(unknown),
    m_calls_OpenWindow(0),
    m_calls_CloseWindow(0),
    m_lastArg_OpenWindow_delta(unknown),
    m_lastArg_CloseWindow_delta(unknown)
  {
  }

  bool ReadDhtSensor() { return m_mock_ReadDhtSensor; }
  float Temperature() const { return m_mock_Temperature; }
  float Humidity() const { return m_mock_Humidity; }

  void OpenWindow(float delta)
  {
    m_calls_OpenWindow++;
    m_lastArg_OpenWindow_delta = delta;
  }

  void CloseWindow(float delta)
  {
    m_calls_CloseWindow++;
    m_lastArg_CloseWindow_delta = delta;
  }

  // expose protected members to public
  void AutoMode(bool autoMode) { Greenhouse::AutoMode(autoMode); }
  void OpenStart(float openStart) { Greenhouse::OpenStart(openStart); }
  void OpenFinish(float openFinish) { Greenhouse::OpenFinish(openFinish); }
  void WindowProgress(int windowProgress) { Greenhouse::WindowProgress(windowProgress); }

  bool m_mock_ReadDhtSensor;
  float m_mock_Temperature;
  float m_mock_Humidity;
  int m_calls_OpenWindow;
  int m_calls_CloseWindow;
  float m_lastArg_OpenWindow_delta;
  float m_lastArg_CloseWindow_delta;
};

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

void Test_Refresh_AutoModeAndNoOpenStart_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 1;
  greenhouse.AutoMode(true);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAndAboveOpenStart_WindowOpens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 1.1;
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(0);
  greenhouse.OpenStart(1.0);
  greenhouse.OpenFinish(2.0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAndBelowOpenStart_WindowCloses(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 0.9;
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(1);
  greenhouse.OpenStart(1.0);
  greenhouse.OpenFinish(2.0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAndAboveOpenStartAndAlreadyOpen_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 2.0;
  greenhouse.AutoMode(true);
  greenhouse.OpenStart(1.0);
  greenhouse.OpenFinish(2.0);
  greenhouse.WindowProgress(100);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAndBelowOpenStartAndAlreadyClosed_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 0.9;
  greenhouse.AutoMode(true);
  greenhouse.OpenStart(1.0);
  greenhouse.OpenFinish(2.0);
  greenhouse.WindowProgress(0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAndBetweenBoundsAndAboveProgress_WindowOpenedPartly(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 1.6; // 60% between
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(50); // 50% open
  greenhouse.OpenStart(1.0);
  greenhouse.OpenFinish(2.0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.1, greenhouse.m_lastArg_OpenWindow_delta); // 10% delta
}

void Test_Refresh_AutoModeAndBetweenBoundsAndBelowProgress_WindowClosedPartly(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_Temperature = 1.4; // 40% between
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(50); // 50% open
  greenhouse.OpenStart(1.0);
  greenhouse.OpenFinish(2.0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.1, greenhouse.m_lastArg_CloseWindow_delta); // 10% delta
}

void process()
{
  UNITY_BEGIN();
  RUN_TEST(Test_Refresh_DhtNotReady_NothingHappens);
  RUN_TEST(Test_Refresh_ManualMode_NothingHappens);
  RUN_TEST(Test_Refresh_AutoModeAndNoOpenStart_NothingHappens);
  RUN_TEST(Test_Refresh_AutoModeAndAboveOpenStart_WindowOpens);
  RUN_TEST(Test_Refresh_AutoModeAndBelowOpenStart_WindowCloses);
  RUN_TEST(Test_Refresh_AutoModeAndAboveOpenStartAndAlreadyOpen_NothingHappens);
  RUN_TEST(Test_Refresh_AutoModeAndBelowOpenStartAndAlreadyClosed_NothingHappens);
  RUN_TEST(Test_Refresh_AutoModeAndBetweenBoundsAndAboveProgress_WindowOpenedPartly);
  RUN_TEST(Test_Refresh_AutoModeAndBetweenBoundsAndBelowProgress_WindowClosedPartly);
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
