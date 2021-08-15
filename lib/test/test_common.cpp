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
    m_calls_SwitchWaterBattery(0),
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
  float CalculateMoisture(float analogValue) const { return Greenhouse::CalculateMoisture(analogValue); }
  void UpdateWaterBattery() { Greenhouse::UpdateWaterBattery(); }
  void SwitchWaterBattery(bool on) { m_calls_SwitchWaterBattery++; m_lastArg_SwitchWaterBattery_on = on; }
  void WaterBatteryOnHour(int value) { Greenhouse::WaterBatteryOnHour(value); }
  void WaterBatteryOffHour(int value) { Greenhouse::WaterBatteryOffHour(value); }

  bool ApplyWindowProgress(float expectedProgress)
  {
    return Greenhouse::ApplyWindowProgress(expectedProgress);
  }

  bool m_mock_ReadDhtSensor;
  float m_mock_SoilTemperature;
  int m_mock_CurrentHour;

  int m_calls_OpenWindow;
  int m_calls_CloseWindow;
  int m_calls_SwitchWaterBattery;

  float m_lastArg_OpenWindow_delta;
  float m_lastArg_CloseWindow_delta;
  bool m_lastArg_SwitchWaterBattery_on;
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
  greenhouse.m_mock_SoilTemperature = 2.1;
  greenhouse.AutoMode(true);
  greenhouse.OpenStart(1.1);
  greenhouse.OpenFinish(2.1);
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
  greenhouse.OpenStart(1.1);
  greenhouse.OpenFinish(2.1);
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
  greenhouse.WindowProgress(50);
  greenhouse.OpenStart(25.1);
  greenhouse.OpenFinish(30.1);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.27, greenhouse.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(77, greenhouse.WindowProgress());
}

void Test_Refresh_AutoModeInBoundsTooOpen_WindowClosedPartly(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 25 + (5 * 0.4); // 40% between 25C and 30C
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(80);
  greenhouse.OpenStart(25.1);
  greenhouse.OpenFinish(30.1);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.42, greenhouse.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(37, greenhouse.WindowProgress());
}

void Test_Refresh_AutoModeInBoundsTooOpenTwice_WindowClosedPartlyTwice(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 25 + (5 * 0.6); // 60% between 25C and 30C
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(90);
  greenhouse.OpenStart(25.1);
  greenhouse.OpenFinish(30.1);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.32, greenhouse.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(57, greenhouse.WindowProgress());

  greenhouse.m_mock_SoilTemperature = 25 + (5 * 0.39); // 40% between 25C and 30C
  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(2, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.2, greenhouse.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(37, greenhouse.WindowProgress());
}

void Test_Refresh_AutoModeInBoundsTooClosedTwice_WindowOpenedPartlyTwice(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 25 + (5 * 0.3); // 30% between 25C and 30C
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(5);
  greenhouse.OpenStart(25.1);
  greenhouse.OpenFinish(30.1);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.22, greenhouse.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(27, greenhouse.WindowProgress());

  greenhouse.m_mock_SoilTemperature = 29; // 80% between 25C and 30C
  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(2, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.50, greenhouse.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(77, greenhouse.WindowProgress());
}

void Test_Refresh_AutoModeBelowBounds_WindowClosedFully(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 24.9;
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(30);
  greenhouse.OpenStart(25.1);
  greenhouse.OpenFinish(30.1);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.3, greenhouse.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(0, greenhouse.WindowProgress());
}

void Test_Refresh_AutoModeAboveBounds_WindowOpenedFully(void)
{
  GreenhouseTest greenhouse;
  greenhouse.m_mock_ReadDhtSensor = true;
  greenhouse.m_mock_SoilTemperature = 30.1;
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(60);
  greenhouse.OpenStart(25.1);
  greenhouse.OpenFinish(30.1);

  greenhouse.Refresh();

  TEST_ASSERT_EQUAL_INT(0, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.4, greenhouse.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(100, greenhouse.WindowProgress());
}

void Test_ApplyWindowProgress_CloseNoOpenDayMinimumInDay_FullyClosed()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(20);
  greenhouse.OpenDayMinimum(0);
  greenhouse.m_mock_CurrentHour = 12; // 12pm

  greenhouse.ApplyWindowProgress(0);

  TEST_ASSERT_EQUAL_INT(0, greenhouse.WindowProgress());
}

void Test_ApplyWindowProgress_CloseWithOpenDayMinimumAtNight_FullyClosed()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(20);
  greenhouse.OpenDayMinimum(10);
  greenhouse.m_mock_CurrentHour = 0; // 12am

  greenhouse.ApplyWindowProgress(0);

  TEST_ASSERT_EQUAL_INT(0, greenhouse.WindowProgress());
}

void Test_ApplyWindowProgress_CloseWithOpenDayMinimumInDay_PartlyClosed()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(20);
  greenhouse.OpenDayMinimum(10);
  greenhouse.m_mock_CurrentHour = 12; // 12pm

  greenhouse.ApplyWindowProgress(0);

  TEST_ASSERT_EQUAL_INT(10, greenhouse.WindowProgress());
}

void Test_ApplyWindowProgress_OpenDayMinimumAboveProgress_PartlyOpened()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(5);
  greenhouse.OpenDayMinimum(10);
  greenhouse.m_mock_CurrentHour = 12; // 12pm

  greenhouse.ApplyWindowProgress(0);

  TEST_ASSERT_EQUAL_INT(10, greenhouse.WindowProgress());
}

void Test_CalculateMoisture_BelowOrEqualMin_ReturnsZero()
{
  GreenhouseTest greenhouse;
  float percent = greenhouse.CalculateMoisture(3.95);
  TEST_ASSERT_EQUAL_FLOAT(0, percent);
}

void Test_CalculateMoisture_AboveOrEqualMax_ReturnsHundred()
{
  GreenhouseTest greenhouse;
  float percent = greenhouse.CalculateMoisture(1.9);
  TEST_ASSERT_EQUAL_FLOAT(100, percent);
}

void Test_CalculateMoisture_InBounds_ReturnsPercent()
{
  GreenhouseTest greenhouse;
  float percent = greenhouse.CalculateMoisture(3);
  TEST_ASSERT_FLOAT_WITHIN(.1, 46.34, percent);
}

void Test_UpdateWaterBattery_CurrentHourBeforeOnHour_SwitchOffCalled()
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 1;
  greenhouse.WaterBatteryOnHour(2);
  greenhouse.WaterBatteryOffHour(4);
  greenhouse.UpdateWaterBattery();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterBattery);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchWaterBattery_on);
}

void Test_UpdateWaterBattery_CurrentHourIsOnHour_SwitchOnCalled()
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 2;
  greenhouse.WaterBatteryOnHour(2);
  greenhouse.WaterBatteryOffHour(4);
  greenhouse.UpdateWaterBattery();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterBattery);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchWaterBattery_on);
}

void Test_UpdateWaterBattery_CurrentHourBetweenOnAndOffHours_SwitchOnCalled()
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 3;
  greenhouse.WaterBatteryOnHour(2);
  greenhouse.WaterBatteryOffHour(4);
  greenhouse.UpdateWaterBattery();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterBattery);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchWaterBattery_on);
}

void Test_UpdateWaterBattery_CurrentHourAtOffHour_SwitchOffCalled()
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 4;
  greenhouse.WaterBatteryOnHour(2);
  greenhouse.WaterBatteryOffHour(4);
  greenhouse.UpdateWaterBattery();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterBattery);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchWaterBattery_on);
}

void Test_UpdateWaterBattery_CurrentHourAfterOffHour_SwitchOffCalled()
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 5;
  greenhouse.WaterBatteryOnHour(2);
  greenhouse.WaterBatteryOffHour(4);
  greenhouse.UpdateWaterBattery();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterBattery);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchWaterBattery_on);
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
  RUN_TEST(Test_CalculateMoisture_BelowOrEqualMin_ReturnsZero);
  RUN_TEST(Test_CalculateMoisture_AboveOrEqualMax_ReturnsHundred);
  RUN_TEST(Test_CalculateMoisture_InBounds_ReturnsPercent);
  RUN_TEST(Test_UpdateWaterBattery_CurrentHourBeforeOnHour_SwitchOffCalled);
  RUN_TEST(Test_UpdateWaterBattery_CurrentHourIsOnHour_SwitchOnCalled);
  RUN_TEST(Test_UpdateWaterBattery_CurrentHourBetweenOnAndOffHours_SwitchOnCalled);
  RUN_TEST(Test_UpdateWaterBattery_CurrentHourAtOffHour_SwitchOffCalled);
  RUN_TEST(Test_UpdateWaterBattery_CurrentHourAfterOffHour_SwitchOffCalled);
}
