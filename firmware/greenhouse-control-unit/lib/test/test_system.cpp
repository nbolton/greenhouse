#include "test.h"

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
  system.WindowOpenPercent(0);
  system.WindowAdjustPositions(10);

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
  system.WindowOpenPercent(0);
  system.WindowAdjustPositions(10);

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
  system.WindowOpenPercent(0);
  system.WindowAdjustPositions(10);

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
  system.WindowOpenPercent(100);
  system.UpdateWindowOpenPercent();
  system.WindowAdjustPositions(10);

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
  system.WindowOpenPercent(0);
  system.UpdateWindowOpenPercent();
  system.WindowAdjustPositions(10);

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
  system.WindowOpenPercent(50);
  system.UpdateWindowOpenPercent();
  system.OpenStart(25.1);
  system.OpenFinish(30.1);
  system.WindowAdjustPositions(10);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.3, system.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(80, system.WindowOpenPercentActual());
}

void Test_Refresh_AutoModeInBoundsTooOpen_WindowClosedPartly(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 25 + (5 * 0.4); // 40% between 25C and 30C
  system.AutoMode(true);
  system.WindowOpenPercent(80);
  system.UpdateWindowOpenPercent();
  system.OpenStart(25.1);
  system.OpenFinish(30.1);
  system.WindowAdjustPositions(10);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.4, system.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(40, system.WindowOpenPercentActual());
}

void Test_Refresh_AutoModeInBoundsTooOpenTwice_WindowClosedPartlyTwice(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 25 + (5 * 0.6); // 60% between 25C and 30C
  system.AutoMode(true);
  system.WindowOpenPercent(90);
  system.UpdateWindowOpenPercent();
  system.OpenStart(25.1);
  system.OpenFinish(30.1);
  system.WindowAdjustPositions(10);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.3, system.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(60, system.WindowOpenPercentActual());

  system.m_mock_SoilTemperature = 25 + (5 * 0.39); // 40% between 25C and 30C
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(2, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.2, system.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(40, system.WindowOpenPercentActual());
}

void Test_Refresh_AutoModeInBoundsTooClosedTwice_WindowOpenedPartlyTwice(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 25 + (5 * 0.3); // 30% between 25C and 30C
  system.AutoMode(true);
  system.WindowOpenPercent(5);
  system.UpdateWindowOpenPercent();
  system.OpenStart(25.1);
  system.OpenFinish(30.1);
  system.WindowAdjustPositions(10);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.25, system.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(30, system.WindowOpenPercentActual());

  system.m_mock_SoilTemperature = 29; // 80% between 25C and 30C
  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(2, system.m_calls_OpenWindow);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.50, system.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(80, system.WindowOpenPercentActual());
}

void Test_Refresh_AutoModeBelowBounds_WindowClosedFully(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 24.9;
  system.AutoMode(true);
  system.WindowOpenPercent(30);
  system.UpdateWindowOpenPercent();
  system.OpenStart(25.1);
  system.OpenFinish(30.1);
  system.WindowAdjustPositions(10);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.3, system.m_lastArg_CloseWindow_delta);
  TEST_ASSERT_EQUAL_INT(0, system.WindowOpenPercentActual());
}

void Test_Refresh_AutoModeAboveBounds_WindowOpenedFully(void)
{
  TestSystem system;
  system.m_mock_ReadDhtSensor = true;
  system.m_mock_SoilTemperature = 30.1;
  system.AutoMode(true);
  system.WindowOpenPercent(60);
  system.UpdateWindowOpenPercent();
  system.OpenStart(25.1);
  system.OpenFinish(30.1);
  system.WindowAdjustPositions(10);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(0.4, system.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_INT(100, system.WindowOpenPercentActual());
}

void Test_Refresh_RainDetectedInAutoMode_WindowClosed(void)
{
  TestSystem system;
  system.AutoMode(true);
  system.WindowOpenPercent(50);
  system.UpdateWindowOpenPercent();
  system.WeatherCode(700);
  system.WindowAdjustPositions(10);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_FLOAT(.5, system.m_lastArg_CloseWindow_delta);
}

void Test_Refresh_RainDetectedInManualMode_WindowClosed(void)
{
  TestSystem system;
  system.AutoMode(false);
  system.WindowOpenPercent(50);
  system.UpdateWindowOpenPercent();
  system.WeatherCode(700);
  system.WindowAdjustPositions(10);

  system.Refresh();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL_FLOAT(.5, system.m_lastArg_CloseWindow_delta);
}

void Test_Loop_WindowAdjustOnceUnderThreshold_NothingHappens(void)
{
  TestSystem system;
  system.WindowAdjustPositions(5); // i.e. first is 20%
  system.WindowOpenPercent(0); // 0% open
  system.UpdateWindowOpenPercent();

  system.WindowOpenPercent(5); // 5% open (round to 0%)
  system.UpdateWindowOpenPercent();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(5, system.WindowOpenPercentExpected());
  TEST_ASSERT_EQUAL_FLOAT(0, system.WindowOpenPercentActual());
}

void Test_Loop_WindowAdjustOnceOverThreshold_WindowOpens(void)
{
  TestSystem system;
  system.WindowAdjustPositions(5); // i.e. first is 20%
  system.WindowOpenPercent(0); // 0% open
  system.UpdateWindowOpenPercent();

  system.WindowOpenPercent(25); // 25% open (round to 20%)
  system.UpdateWindowOpenPercent();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(.2, system.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_FLOAT(25, system.WindowOpenPercentExpected());
  TEST_ASSERT_EQUAL_FLOAT(20, system.WindowOpenPercentActual());
}

void Test_Loop_WindowAdjustTwiceUnderThenOverThreshold_WindowOpens(void)
{
  TestSystem system;
  system.WindowAdjustPositions(5); // i.e. first is 20%
  system.WindowOpenPercent(20); // 0% open
  system.UpdateWindowOpenPercent();

  system.WindowOpenPercent(25); // 25% open (round to 20%)
  system.UpdateWindowOpenPercent();

  TEST_ASSERT_EQUAL_INT(0, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(25, system.WindowOpenPercentExpected());
  TEST_ASSERT_EQUAL_FLOAT(20, system.WindowOpenPercentActual());

  system.WindowOpenPercent(35); // 35% open (round to 40%)
  system.UpdateWindowOpenPercent();

  TEST_ASSERT_EQUAL_INT(1, system.m_calls_OpenWindow);
  TEST_ASSERT_EQUAL_FLOAT(.2, system.m_lastArg_OpenWindow_delta);
  TEST_ASSERT_EQUAL_FLOAT(35, system.WindowOpenPercentExpected());
  TEST_ASSERT_EQUAL_FLOAT(40, system.WindowOpenPercentActual());
}

void Test_OpenWindow_HalfDelta_ActuatorMovedForwardHalf(void)
{
  TestSystem system;
  TestTime time;
  system.m_mock_time = &time;

  time.m_mock_EpochTime = 0;
  system.WindowActuatorRuntimeSec(2);

  system.OpenWindow(.5);
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_RunWindowActuator);
  
  system.Loop();
  TEST_ASSERT_EQUAL(true, system.m_lastArg_RunWindowActuator_extend);
}

void Test_CloseWindow_HalfDelta_ActuatorMovedBackwardHalf(void)
{
  TestSystem system;
  TestTime time;
  system.m_mock_time = &time;

  time.m_mock_EpochTime = 0;
  system.WindowActuatorRuntimeSec(2);

  system.CloseWindow(.5);
  system.Loop();
  TEST_ASSERT_EQUAL_INT(1, system.m_calls_RunWindowActuator);

  system.Loop();
  TEST_ASSERT_EQUAL(false, system.m_lastArg_RunWindowActuator_extend);
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

void Test_SoilMoistureAverage_SingleValue_CorrectValue()
{
  TestSystem system;
  system.SoilMoistureSampleMax(1);
  system.AddSoilMoistureSample(1);
  TEST_ASSERT_EQUAL_FLOAT(1, system.SoilMoistureAverage());
}

void Test_SoilMoistureAverage_TwoValues_CorrectValue()
{
  TestSystem system;
  system.SoilMoistureSampleMax(2);
  system.AddSoilMoistureSample(1);
  system.AddSoilMoistureSample(2);
  TEST_ASSERT_EQUAL_FLOAT(1.5, system.SoilMoistureAverage());
}

void Test_SoilMoistureAverage_ValueAddedUnderSampleMax_CorrectValue()
{
  TestSystem system;
  system.SoilMoistureSampleMax(3);
  system.AddSoilMoistureSample(1);
  system.AddSoilMoistureSample(2);
  system.AddSoilMoistureSample(3);
  TEST_ASSERT_EQUAL_FLOAT(2, system.SoilMoistureAverage());
}

void Test_SoilMoistureAverage_ValueAddedOverSampleMax3_AverageLast3()
{
  TestSystem system;
  system.SoilMoistureSampleMax(3);
  system.AddSoilMoistureSample(1);
  system.AddSoilMoistureSample(2);
  system.AddSoilMoistureSample(3);
  system.AddSoilMoistureSample(4);
  TEST_ASSERT_EQUAL_FLOAT(3, system.SoilMoistureAverage());
}

void Test_SoilCalibrateWet_SamplesAdded_AverageIsReset()
{
  TestSystem system;
  system.SoilMoistureSampleMax(3);
  system.AddSoilMoistureSample(1);
  system.AddSoilMoistureSample(2);
  system.m_mock_ReadSoilMoistureSensor = true;
  TEST_ASSERT_EQUAL_FLOAT(1.5, system.SoilMoistureAverage());

  system.SoilCalibrateWet();
  system.AddSoilMoistureSample(1);
  TEST_ASSERT_EQUAL_FLOAT(1, system.SoilMoistureAverage());
}

void Test_SoilCalibrateDry_SamplesAdded_AverageIsReset()
{
  TestSystem system;
  system.SoilMoistureSampleMax(3);
  system.AddSoilMoistureSample(1);
  system.AddSoilMoistureSample(2);
  system.m_mock_ReadSoilMoistureSensor = true;
  TEST_ASSERT_EQUAL_FLOAT(1.5, system.SoilMoistureAverage());

  system.SoilCalibrateDry();
  system.AddSoilMoistureSample(1);
  TEST_ASSERT_EQUAL_FLOAT(1, system.SoilMoistureAverage());
}

void testSystem()
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
  RUN_TEST(Test_Refresh_RainDetectedInAutoMode_WindowClosed);
  RUN_TEST(Test_Refresh_RainDetectedInManualMode_WindowClosed);
  RUN_TEST(Test_Loop_WindowAdjustOnceUnderThreshold_NothingHappens);
  RUN_TEST(Test_Loop_WindowAdjustOnceOverThreshold_WindowOpens);
  RUN_TEST(Test_Loop_WindowAdjustTwiceUnderThenOverThreshold_WindowOpens);
  RUN_TEST(Test_OpenWindow_HalfDelta_ActuatorMovedForwardHalf);
  RUN_TEST(Test_CloseWindow_HalfDelta_ActuatorMovedBackwardHalf);
  RUN_TEST(Test_CalculateMoisture_BelowOrEqualMin_ReturnsZero);
  RUN_TEST(Test_CalculateMoisture_AboveOrEqualMax_ReturnsHundred);
  RUN_TEST(Test_CalculateMoisture_InBounds_ReturnsPercent);
  RUN_TEST(Test_CalculateMoisture_AboveBounds_ReturnsUnknown);
  RUN_TEST(Test_CalculateMoisture_BelowBounds_ReturnsUnknown);
  RUN_TEST(Test_SoilMoistureAverage_SingleValue_CorrectValue);
  RUN_TEST(Test_SoilMoistureAverage_TwoValues_CorrectValue);
  RUN_TEST(Test_SoilMoistureAverage_ValueAddedUnderSampleMax_CorrectValue);
  RUN_TEST(Test_SoilMoistureAverage_ValueAddedOverSampleMax3_AverageLast3);
  RUN_TEST(Test_SoilCalibrateWet_SamplesAdded_AverageIsReset);
  RUN_TEST(Test_SoilCalibrateDry_SamplesAdded_AverageIsReset);
}
