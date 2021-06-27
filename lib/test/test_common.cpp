#include "test_common.h"

#include "Greenhouse.h"
#include <unity.h>

class GreenhouseTest : public Greenhouse {
public:
  GreenhouseTest() :
    m_mock_ReadDhtSensor(false),
    m_mock_SoilTemperature(k_unknown),
    m_mock_CurrentHour(k_unknown),
    m_calls_OpenWindow(0),
    m_calls_CloseWindow(0),
    m_lastArg_OpenWindow_delta(k_unknown),
    m_lastArg_CloseWindow_delta(k_unknown)
  {
  }

  bool ReadSensors() { return m_mock_ReadDhtSensor; }
  float SoilTemperature() const { return m_mock_SoilTemperature; }
  int CurrentHour() const { return m_mock_CurrentHour; }

  void OpenWindow(float delta)
  {
    Log().Trace("Mock OpenWindow, delta=%.2f", delta);
    m_calls_OpenWindow++;
    m_lastArg_OpenWindow_delta = delta;

    Greenhouse::OpenWindow(delta);
  }

  void CloseWindow(float delta)
  {
    Log().Trace("Mock CloseWindow, delta=%.2f", delta);
    m_calls_CloseWindow++;
    m_lastArg_CloseWindow_delta = delta;

    Greenhouse::CloseWindow(delta);
  }

  // expose protected members to public
  void AutoMode(bool autoMode) { Greenhouse::AutoMode(autoMode); }
  void OpenStart(float openStart) { Greenhouse::OpenStart(openStart); }
  void OpenFinish(float openFinish) { Greenhouse::OpenFinish(openFinish); }
  void WindowProgress(int windowProgress) { Greenhouse::WindowProgress(windowProgress); }
  int WindowProgress() { return Greenhouse::WindowProgress(); }
  void OpenDayMinimum(int openDayMinimum) { Greenhouse::OpenDayMinimum(openDayMinimum); }

  bool ApplyWindowProgress(float expectedProgress)
  {
    return Greenhouse::ApplyWindowProgress(expectedProgress);
  }

  bool m_mock_ReadDhtSensor;
  float m_mock_SoilTemperature;
  int m_mock_CurrentHour;

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
  greenhouse.m_mock_SoilTemperature = 1;
  greenhouse.AutoMode(false);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeNoOpenBounds_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 1;
  greenhouse.AutoMode(true);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAboveBoundsAndAlreadyOpen_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 2.0;
  greenhouse.AutoMode(true);
  greenhouse.OpenStart(1.0);
  greenhouse.OpenFinish(2.0);
  greenhouse.WindowProgress(100);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeBelowBoundsAndAlreadyClosed_NothingHappens(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 0.9;
  greenhouse.AutoMode(true);
  greenhouse.OpenStart(1.0);
  greenhouse.OpenFinish(2.0);
  greenhouse.WindowProgress(0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeInBoundsTooClosed_WindowOpenedPartly(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 25 + (5 * 0.8); // 80% between 25C and 30C
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(50); // 50% open
  greenhouse.OpenStart(25.0);
  greenhouse.OpenFinish(30.0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.3, greenhouse.m_lastArg_OpenWindow_delta); // 20% delta
  TEST_ASSERT_EQUAL_INT(80, greenhouse.WindowProgress());              // 80% open
}

void Test_Refresh_AutoModeInBoundsTooOpen_WindowClosedPartly(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 25 + (5 * 0.4); // 40% between 25C and 30C
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(80); // 80% open
  greenhouse.OpenStart(25.0);
  greenhouse.OpenFinish(30.0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.4, greenhouse.m_lastArg_CloseWindow_delta); // 40% delta
  TEST_ASSERT_EQUAL_INT(40, greenhouse.WindowProgress());               // 40% open
}

void Test_Refresh_AutoModeInBoundsTooOpenTwice_WindowClosedPartlyTwice(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 25 + (5 * 0.6); // 60% between 25C and 30C
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(90); // 90% open
  greenhouse.OpenStart(25.0);
  greenhouse.OpenFinish(30.0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.3, greenhouse.m_lastArg_CloseWindow_delta); // 30% delta
  TEST_ASSERT_EQUAL_INT(60, greenhouse.WindowProgress());               // 60% open

  greenhouse.m_mock_SoilTemperature = 25 + (5 * 0.4); // 40% between 25C and 30C
  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(2, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.2, greenhouse.m_lastArg_CloseWindow_delta); // 20% delta
  TEST_ASSERT_EQUAL_INT(40, greenhouse.WindowProgress());               // 40% open
}

void Test_Refresh_AutoModeInBoundsTooClosedTwice_WindowOpenedPartlyTwice(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 25 + (5 * 0.3); // 30% between 25C and 30C
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(5); // 5% open
  greenhouse.OpenStart(25.0);
  greenhouse.OpenFinish(30.0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.25, greenhouse.m_lastArg_OpenWindow_delta); // 25% delta
  TEST_ASSERT_EQUAL_INT(30, greenhouse.WindowProgress());               // 30% open

  greenhouse.m_mock_SoilTemperature = 29; // 80% between 25C and 30C
  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(2, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.50, greenhouse.m_lastArg_OpenWindow_delta); // 55% delta
  TEST_ASSERT_EQUAL_INT(80, greenhouse.WindowProgress());               // 80% open
}

void Test_Refresh_AutoModeBelowBounds_WindowClosedFully(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 24.9;
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(30); // 30% open
  greenhouse.OpenStart(25.0);
  greenhouse.OpenFinish(30.0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.3, greenhouse.m_lastArg_CloseWindow_delta); // 30% delta
  TEST_ASSERT_EQUAL_INT(0, greenhouse.WindowProgress());                // 0% open
}

void Test_Refresh_AutoModeAboveBounds_WindowOpenedFully(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 30.1;
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(60); // 60% open
  greenhouse.OpenStart(25.0);
  greenhouse.OpenFinish(30.0);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.4, greenhouse.m_lastArg_OpenWindow_delta); // 40% delta
  TEST_ASSERT_EQUAL_INT(100, greenhouse.WindowProgress());             // 100% open
}

void Test_ApplyWindowProgress_CloseNoOpenDayMinimumInDay_FullyClosed()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(20);      // 20% open
  greenhouse.OpenDayMinimum(0);       // 0% min
  greenhouse.m_mock_CurrentHour = 12; // 12pm

  greenhouse.ApplyWindowProgress(0);

  TEST_ASSERT_EQUAL_INT(0, greenhouse.WindowProgress()); // 0% open
}

void Test_ApplyWindowProgress_CloseWithOpenDayMinimumAtNight_FullyClosed()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(20);     // 20% open
  greenhouse.OpenDayMinimum(10);     // 10% min
  greenhouse.m_mock_CurrentHour = 0; // 12am

  greenhouse.ApplyWindowProgress(0);

  TEST_ASSERT_EQUAL_INT(0, greenhouse.WindowProgress()); // 0% open
}

void Test_ApplyWindowProgress_CloseWithOpenDayMinimumInDay_PartlyClosed()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(20);      // 20% open
  greenhouse.OpenDayMinimum(10);      // 10% min
  greenhouse.m_mock_CurrentHour = 12; // 12pm

  greenhouse.ApplyWindowProgress(0);

  TEST_ASSERT_EQUAL_INT(10, greenhouse.WindowProgress()); // 10% open
}

void Test_ApplyWindowProgress_OpenDayMinimumAboveProgress_PartlyOpened()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(5);       // 5% open
  greenhouse.OpenDayMinimum(10);      // 10% min
  greenhouse.m_mock_CurrentHour = 12; // 12pm

  greenhouse.ApplyWindowProgress(0);

  TEST_ASSERT_EQUAL_INT(10, greenhouse.WindowProgress()); // 10% open
}

void testCommon()
{
  RUN_TEST(Test_Refresh_DhtNotReady_NothingHappens);
  RUN_TEST(Test_Refresh_ManualMode_NothingHappens);
  RUN_TEST(Test_Refresh_AutoModeNoOpenBounds_NothingHappens);
  RUN_TEST(Test_Refresh_AutoModeAboveBoundsAndAlreadyOpen_NothingHappens);
  RUN_TEST(Test_Refresh_AutoModeBelowBoundsAndAlreadyClosed_NothingHappens);
  RUN_TEST(Test_Refresh_AutoModeInBoundsTooClosed_WindowOpenedPartly);
  RUN_TEST(Test_Refresh_AutoModeInBoundsTooOpen_WindowClosedPartly);
  RUN_TEST(Test_Refresh_AutoModeInBoundsTooClosedTwice_WindowOpenedPartlyTwice);
  RUN_TEST(Test_Refresh_AutoModeInBoundsTooOpenTwice_WindowClosedPartlyTwice);
  RUN_TEST(Test_Refresh_AutoModeBelowBounds_WindowClosedFully);
  RUN_TEST(Test_Refresh_AutoModeAboveBounds_WindowOpenedFully);
  RUN_TEST(Test_ApplyWindowProgress_CloseNoOpenDayMinimumInDay_FullyClosed);
  RUN_TEST(Test_ApplyWindowProgress_CloseWithOpenDayMinimumAtNight_FullyClosed);
  RUN_TEST(Test_ApplyWindowProgress_CloseWithOpenDayMinimumInDay_PartlyClosed);
  RUN_TEST(Test_ApplyWindowProgress_OpenDayMinimumAboveProgress_PartlyOpened);
}
