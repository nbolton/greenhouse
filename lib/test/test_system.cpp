#include "test_system.h"

#include "TestSystem.h"
#include <unity.h>

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void Test_Refresh_DhtNotReady_NothingHappens(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = false;

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
}

void Test_Refresh_ManualMode_NothingHappens(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 1;
  system.AutoMode(false);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeNoOpenBounds_NothingHappens(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 1;
  system.AutoMode(true);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeAboveBoundsAndAlreadyOpen_NothingHappens(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 2.1;
  system.AutoMode(true);
  system.OpenStart(1.1);
  system.OpenFinish(2.1);
  system.WindowProgress(100);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeBelowBoundsAndAlreadyClosed_NothingHappens(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 0.9;
  system.AutoMode(true);
  system.OpenStart(1.1);
  system.OpenFinish(2.1);
  system.WindowProgress(0);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
}

void Test_Refresh_AutoModeInBoundsTooClosed_WindowOpenedPartly(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 25 + (5 * 0.8); // 80% between 25C and 30C
  system.AutoMode(true);
  system.WindowProgress(50);
  system.OpenStart(25.1);
  system.OpenFinish(30.1);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.27, system.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(77, system.WindowProgress());
}

void Test_Refresh_AutoModeInBoundsTooOpen_WindowClosedPartly(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 25 + (5 * 0.4); // 40% between 25C and 30C
  system.AutoMode(true);
  system.WindowProgress(80);
  system.OpenStart(25.1);
  system.OpenFinish(30.1);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.42, system.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(37, system.WindowProgress());
}

void Test_Refresh_AutoModeInBoundsTooOpenTwice_WindowClosedPartlyTwice(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 25 + (5 * 0.6); // 60% between 25C and 30C
  system.AutoMode(true);
  system.WindowProgress(90);
  system.OpenStart(25.1);
  system.OpenFinish(30.1);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.32, system.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(57, system.WindowProgress());

  system.m_mock_SoilTemperature = 25 + (5 * 0.39); // 40% between 25C and 30C
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(2, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.2, system.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(37, system.WindowProgress());
}

void Test_Refresh_AutoModeInBoundsTooClosedTwice_WindowOpenedPartlyTwice(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 25 + (5 * 0.3); // 30% between 25C and 30C
  system.AutoMode(true);
  system.WindowProgress(5);
  system.OpenStart(25.1);
  system.OpenFinish(30.1);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.22, system.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(27, system.WindowProgress());

  system.m_mock_SoilTemperature = 29; // 80% between 25C and 30C
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(2, system.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.50, system.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(77, system.WindowProgress());
}

void Test_Refresh_AutoModeBelowBounds_WindowClosedFully(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 24.9;
  system.AutoMode(true);
  system.WindowProgress(30);
  system.OpenStart(25.1);
  system.OpenFinish(30.1);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.3, system.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(0, system.WindowProgress());
}

void Test_Refresh_AutoModeAboveBounds_WindowOpenedFully(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 30.1;
  system.AutoMode(true);
  system.WindowProgress(60);
  system.OpenStart(25.1);
  system.OpenFinish(30.1);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.4, system.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(100, system.WindowProgress());
}

void Test_Refresh_RainDetectedInAutoMode_WindowClosed(void)
{
  TestSystem system;
  system.AutoMode(true);
  system.WindowProgress(50);
  system.WeatherCode(700);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL(.5, system.m_lastArg_CloseWindow_delta);
}

void Test_Refresh_RainDetectedInManualMode_WindowClosed(void)
{
  TestSystem system;
  system.AutoMode(false);
  system.WindowProgress(50);
  system.WeatherCode(700);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL(.5, system.m_lastArg_CloseWindow_delta);
}

void Test_Refresh_NightDayNightDay_DayNightTransitionedTwice(void)
{
  TestSystem system;
  
  system.m_mock_CurrentHour = 1;
  system.m_mock_EpochTime = 86400L * 1; // +1 day

  system.DayStartHour(2);
  system.DayEndHour(3);

  system.Refresh();
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleDayToNightTransition);

  system.m_mock_CurrentHour = 2;

  system.Refresh();
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleDayToNightTransition);

  system.m_mock_CurrentHour = 1;
  system.m_mock_EpochTime = 86400L * 2; // +1 day

  system.Refresh();
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(2, system.m_calls_HandleDayToNightTransition);

  system.m_mock_CurrentHour = 2;

  system.Refresh();
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(2, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(2, system.m_calls_HandleDayToNightTransition);
}

void Test_Refresh_DayNightDayNight_DayNightTransitionedTwice(void)
{
  TestSystem system;
  
  system.m_mock_CurrentHour = 2;
  system.m_mock_EpochTime = 86400L * 1; // +1 day

  system.DayStartHour(2);
  system.DayEndHour(3);

  system.Refresh();
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_HandleDayToNightTransition);

  system.m_mock_CurrentHour = 1;
  system.m_mock_EpochTime = 86400L * 2; // +1 day

  system.Refresh();
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleDayToNightTransition);

  system.m_mock_CurrentHour = 2;

  system.Refresh();
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(2, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleDayToNightTransition);

  system.m_mock_CurrentHour = 1;
  system.m_mock_EpochTime = 86400L * 3; // +1 day

  system.Refresh();
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(2, system.m_calls_HandleNightToDayTransition);
  TEST_ASSERT_EQUAL_INT(2, system.m_calls_HandleDayToNightTransition);
}

void Test_Refresh_LastNightTransitionSameDay_TransitionedToNight(void)
{
  TestSystem system;
  
  system.m_mock_CurrentHour = 22;
  system.m_mock_EpochTime = 1649365645;
  system.DayEndHour(17);

  system.NightToDayTransitionTime(1649314854);
  system.DayToNightTransitionTime(1649289653);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_HandleDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(1649365645, system.DayToNightTransitionTime());
}

void Test_Refresh_LastNightTransitionSameDay_NextHourNoTransition(void)
{
  TestSystem system;
  
  system.m_mock_CurrentHour = 23;
  system.m_mock_EpochTime = 1649365645 + 3600;

  system.NightToDayTransitionTime(1649314854);
  system.DayToNightTransitionTime(1649365645);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_HandleDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(1649365645, system.DayToNightTransitionTime());
}

void Test_Refresh_LastNightTransitionSameDay_NextDayNoTransition(void)
{
  TestSystem system;
  
  system.m_mock_CurrentHour = 00;
  system.m_mock_EpochTime = 1649365645 + (3600 * 2);

  system.NightToDayTransitionTime(1649314854);
  system.DayToNightTransitionTime(1649365645);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_HandleDayToNightTransition);
  TEST_ASSERT_EQUAL_INT(1649365645, system.DayToNightTransitionTime());
}

void Test_OpenWindow_HalfDelta_ActuatorMovedForwardHalf(void)
{
  TestSystem system;
  system.WindowActuatorRuntimeSec(1.1);

  system.OpenWindow(.5);

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_RunWindowActuator);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_StopActuator);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_SystemDelay);

  TEST_ASSERT_EQUAL(true, system.m_lastArg_RunWindowActuator_extend);
  TEST_ASSERT_EQUAL_UINT64(550, system.m_lastArg_SystemDelay_ms);
}

void Test_CloseWindow_HalfDelta_ActuatorMovedBackwardHalf(void)
{
  TestSystem system;
  system.WindowActuatorRuntimeSec(1.1);

  system.CloseWindow(.5);

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_RunWindowActuator);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_StopActuator);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_SystemDelay);

  TEST_ASSERT_EQUAL(false, system.m_lastArg_RunWindowActuator_extend);
  TEST_ASSERT_EQUAL_UINT64(550, system.m_lastArg_SystemDelay_ms);
}

void Test_CalculateMoisture_BelowOrEqualMin_ReturnsZero()
{
  TestSystem system;
  system.SoilSensorDry(2);
  system.SoilSensorWet(1);
  float percent = system.CalculateMoisture(2);
  TEST_ASSERT_EQUAL_FLOAT(0, percent);
}

void Test_CalculateMoisture_AboveOrEqualMax_ReturnsHundred()
{
  TestSystem system;
  system.SoilSensorDry(2);
  system.SoilSensorWet(1);
  float percent = system.CalculateMoisture(1);
  TEST_ASSERT_EQUAL_FLOAT(100, percent);
}

void Test_CalculateMoisture_InBounds_ReturnsPercent()
{
  TestSystem system;
  system.SoilSensorDry(2);
  system.SoilSensorWet(1);
  float percent = system.CalculateMoisture(1.5);
  TEST_ASSERT_FLOAT_WITHIN(.1, 50, percent);
}

void Test_CalculateMoisture_AboveBounds_ReturnsUnknown()
{
  TestSystem system;
  system.SoilSensorDry(2);
  system.SoilSensorWet(1);
  float percent = system.CalculateMoisture(6);
  TEST_ASSERT_EQUAL_INT(k_unknown, percent);
}

void Test_CalculateMoisture_BelowBounds_ReturnsUnknown()
{
  TestSystem system;
  system.SoilSensorDry(2);
  system.SoilSensorWet(1);
  float percent = system.CalculateMoisture(0);
  TEST_ASSERT_EQUAL_INT(k_unknown, percent);
}

void Test_IsDaytime_CurrentHourBeforeDayStart_ReturnsFalse()
{
  TestSystem system;
  system.m_mock_CurrentHour = 0;
  system.DayStartHour(1);
  system.DayEndHour(3);
  TEST_ASSERT_EQUAL(false, system.IsDaytime());
}

void Test_IsDaytime_CurrentHourEqualsDayStart_ReturnsTrue()
{
  TestSystem system;
  system.m_mock_CurrentHour = 1;
  system.DayStartHour(1);
  system.DayEndHour(3);
  TEST_ASSERT_EQUAL(true, system.IsDaytime());
}

void Test_IsDaytime_CurrentHourInBounds_ReturnsTrue()
{
  TestSystem system;
  system.m_mock_CurrentHour = 2;
  system.DayStartHour(1);
  system.DayEndHour(3);
  TEST_ASSERT_EQUAL(true, system.IsDaytime());
}

void Test_IsDaytime_CurrentHourEqualsDayEnd_ReturnsFalse()
{
  TestSystem system;
  system.m_mock_CurrentHour = 3;
  system.DayStartHour(1);
  system.DayEndHour(3);
  TEST_ASSERT_EQUAL(false, system.IsDaytime());
}

void Test_IsDaytime_CurrentHourAfterDayEnd_ReturnsFalse()
{
  TestSystem system;
  system.m_mock_CurrentHour = 4;
  system.DayStartHour(1);
  system.DayEndHour(3);
  TEST_ASSERT_EQUAL(false, system.IsDaytime());
}

void testSystem()
{
  RUN_TEST(Test_Refresh_NightDayNightDay_DayNightTransitionedTwice);
  RUN_TEST(Test_Refresh_DayNightDayNight_DayNightTransitionedTwice);
  RUN_TEST(Test_Refresh_LastNightTransitionSameDay_TransitionedToNight);
  RUN_TEST(Test_Refresh_LastNightTransitionSameDay_NextHourNoTransition);
  RUN_TEST(Test_Refresh_LastNightTransitionSameDay_NextDayNoTransition);
  return;

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
  RUN_TEST(Test_Refresh_RainDetectedInAutoMode_WindowClosed);
  RUN_TEST(Test_Refresh_RainDetectedInManualMode_WindowClosed);
  RUN_TEST(Test_Refresh_NightDayNightDay_DayNightTransitionedTwice);
  RUN_TEST(Test_Refresh_DayNightDayNight_DayNightTransitionedTwice);
  RUN_TEST(Test_Refresh_LastNightTransitionSameDay_TransitionedToNight);
  RUN_TEST(Test_Refresh_LastNightTransitionSameDay_NextHourNoTransition);
  RUN_TEST(Test_Refresh_LastNightTransitionSameDay_NextDayNoTransition);
  RUN_TEST(Test_OpenWindow_HalfDelta_ActuatorMovedForwardHalf);
  RUN_TEST(Test_CloseWindow_HalfDelta_ActuatorMovedBackwardHalf);
  RUN_TEST(Test_CalculateMoisture_BelowOrEqualMin_ReturnsZero);
  RUN_TEST(Test_CalculateMoisture_AboveOrEqualMax_ReturnsHundred);
  RUN_TEST(Test_CalculateMoisture_InBounds_ReturnsPercent);
  RUN_TEST(Test_CalculateMoisture_AboveBounds_ReturnsUnknown);
  RUN_TEST(Test_CalculateMoisture_BelowBounds_ReturnsUnknown);
  RUN_TEST(Test_IsDaytime_CurrentHourBeforeDayStart_ReturnsFalse);
  RUN_TEST(Test_IsDaytime_CurrentHourEqualsDayStart_ReturnsTrue);
  RUN_TEST(Test_IsDaytime_CurrentHourInBounds_ReturnsTrue);
  RUN_TEST(Test_IsDaytime_CurrentHourEqualsDayEnd_ReturnsFalse);
  RUN_TEST(Test_IsDaytime_CurrentHourAfterDayEnd_ReturnsFalse);
}
