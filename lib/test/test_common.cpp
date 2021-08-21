#include "test_common.h"

#include "Greenhouse.h"
#include <unity.h>

class GreenhouseTest : public Greenhouse {
public:
  GreenhouseTest() :

    // calls
    m_calls_OpenWindow(0),
    m_calls_CloseWindow(0),
    m_calls_RunWindowActuator(0),
    m_calls_SwitchWaterHeating(0),
    m_calls_SwitchSoilHeating(0),
    m_calls_SwitchAirHeating(0),
    m_calls_StopActuator(0),
    m_calls_SetWindowActuatorSpeed(0),
    m_calls_SystemDelay(0),

    // mocks
    m_mock_ReadDhtSensor(false),
    m_mock_WaterTemperature(0),
    m_mock_SoilTemperature(0),
    m_mock_InsideAirTemperature(0),
    m_mock_CurrentHour(0),
    m_mock_UptimeSeconds(0)
  {
  }

  // mocks

  bool ReadSensors()
  {
    Log().Trace("Mock: ReadSensors, value=%s", m_mock_ReadDhtSensor ? "true" : "false");
    return m_mock_ReadDhtSensor;
  }

  float WaterTemperature() const
  {
    Log().Trace("Mock: WaterTemperature, value=%.2f", m_mock_WaterTemperature);
    return m_mock_WaterTemperature;
  }

  float SoilTemperature() const
  {
    Log().Trace("Mock: SoilTemperature, value=%.2f", m_mock_SoilTemperature);
    return m_mock_SoilTemperature;
  }

  float InsideAirTemperature() const
  {
    Log().Trace("Mock: InsideAirTemperature, value=%.2f", m_mock_InsideAirTemperature);
    return m_mock_InsideAirTemperature;
  }

  int CurrentHour() const
  {
    Log().Trace("Mock: CurrentHour, value=%d", m_mock_CurrentHour);
    return m_mock_CurrentHour;
  }

  unsigned long UptimeSeconds() const
  {
    Log().Trace("Mock: UptimeSeconds, value=%d", m_mock_UptimeSeconds);
    return m_mock_UptimeSeconds;
  }

  // stubs

  void OpenWindow(float delta)
  {
    Log().Trace("Stub: OpenWindow, delta=%.2f", delta);
    m_calls_OpenWindow++;
    m_lastArg_OpenWindow_delta = delta;

    Greenhouse::OpenWindow(delta);
  }

  void CloseWindow(float delta)
  {
    Log().Trace("Stub: CloseWindow, delta=%.2f", delta);
    m_calls_CloseWindow++;
    m_lastArg_CloseWindow_delta = delta;

    Greenhouse::CloseWindow(delta);
  }

  void RunWindowActuator(bool forward)
  {
    Log().Trace("Stub: RunWindowActuator, forward=%s", forward ? "true" : "false");
    m_lastArg_RunWindowActuator_forward = forward;
    m_calls_RunWindowActuator++;
  }

  void StopActuator()
  {
    Log().Trace("Stub: StopActuator");
    m_calls_StopActuator++;
  }

  void SetWindowActuatorSpeed(int speed)
  {
    Log().Trace("Stub: SetWindowActuatorSpeed, forward=%d", speed);
    m_lastArg_SetWindowActuatorSpeed_speed = speed;
    m_calls_SetWindowActuatorSpeed++;
  }

  void SystemDelay(unsigned long ms)
  {
    Log().Trace("Stub: SystemDelay, forward=%d", ms);
    m_lastArg_SystemDelay_ms = ms;
    m_calls_SystemDelay++;
  }

  void SwitchWaterHeating(bool on)
  {
    Log().Trace("Stub: SwitchWaterHeating, on=%s", on ? "true" : "false");
    m_calls_SwitchWaterHeating++;
    m_lastArg_SwitchWaterHeating_on = on;
  }

  void SwitchSoilHeating(bool on)
  {
    Log().Trace("Stub: SwitchSoilHeating, on=%s", on ? "true" : "false");
    m_calls_SwitchSoilHeating++;
    m_lastArg_SwitchSoilHeating_on = on;
  }

  void SwitchAirHeating(bool on)
  {
    Log().Trace("Stub: SwitchAirHeating, on=%s", on ? "true" : "false");
    m_calls_SwitchAirHeating++;
    m_lastArg_SwitchAirHeating_on = on;
  }
  
  // expose protected members to public

  float CalculateMoisture(float value) const { return Greenhouse::CalculateMoisture(value); }
  void UpdateHeatingSystems() { Greenhouse::UpdateHeatingSystems(); }
  bool ApplyWindowProgress(float value) { return Greenhouse::ApplyWindowProgress(value); }

  // mock values (leave undefined)

  bool m_mock_ReadDhtSensor;
  float m_mock_WaterTemperature;
  float m_mock_SoilTemperature;
  float m_mock_InsideAirTemperature;
  int m_mock_CurrentHour;
  unsigned long m_mock_UptimeSeconds;

  // call counters (init to 0)

  int m_calls_OpenWindow;
  int m_calls_CloseWindow;
  int m_calls_SwitchWaterHeating;
  int m_calls_SwitchSoilHeating;
  int m_calls_SwitchAirHeating;
  int m_calls_RunWindowActuator;
  int m_calls_StopActuator;
  int m_calls_SetWindowActuatorSpeed;
  int m_calls_SystemDelay;

  // last arg (leave undefined)

  float m_lastArg_OpenWindow_delta;
  float m_lastArg_CloseWindow_delta;
  bool m_lastArg_SwitchWaterHeating_on;
  bool m_lastArg_SwitchSoilHeating_on;
  bool m_lastArg_SwitchAirHeating_on;
  bool m_lastArg_RunWindowActuator_forward;
  float m_lastArg_SetWindowActuatorSpeed_speed;
  unsigned long m_lastArg_SystemDelay_ms;
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

void Test_OpenWindow_HalfDelta_ActuatorMovedForwardHalf(void)
{
  GreenhouseTest greenhouse;
  greenhouse.WindowActuatorSpeedPercent(90);
  greenhouse.WindowActuatorRuntimeSec(1.1);

  greenhouse.OpenWindow(.5);

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_RunWindowActuator);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_StopActuator);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SetWindowActuatorSpeed);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SystemDelay);

  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_RunWindowActuator_forward);
  TEST_ASSERT_FLOAT_WITHIN(0.1, 229, greenhouse.m_lastArg_SetWindowActuatorSpeed_speed);
  TEST_ASSERT_EQUAL_UINT64(550, greenhouse.m_lastArg_SystemDelay_ms);
}

void Test_CloseWindow_HalfDelta_ActuatorMovedBackwardHalf(void)
{
  GreenhouseTest greenhouse;
  greenhouse.WindowActuatorSpeedPercent(90);
  greenhouse.WindowActuatorRuntimeSec(1.1);

  greenhouse.CloseWindow(.5);

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_RunWindowActuator);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_StopActuator);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SetWindowActuatorSpeed);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SystemDelay);

  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_RunWindowActuator_forward);
  TEST_ASSERT_FLOAT_WITHIN(0.1, 229, greenhouse.m_lastArg_SetWindowActuatorSpeed_speed);
  TEST_ASSERT_EQUAL_UINT64(550, greenhouse.m_lastArg_SystemDelay_ms);
}

void Test_ApplyWindowProgress_CloseNoOpenDayMinimumInDay_FullyClosed()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(20);
  greenhouse.OpenDayMinimum(0);
  greenhouse.DayStartHour(8);
  greenhouse.DayEndHour(20);
  greenhouse.m_mock_CurrentHour = 12; // 12pm

  greenhouse.ApplyWindowProgress(0);

  TEST_ASSERT_EQUAL_INT(0, greenhouse.WindowProgress());
}

void Test_ApplyWindowProgress_CloseWithOpenDayMinimumAtNight_FullyClosed()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(20);
  greenhouse.OpenDayMinimum(10);
  greenhouse.DayStartHour(8);
  greenhouse.DayEndHour(20);
  greenhouse.m_mock_CurrentHour = 0; // 12am

  greenhouse.ApplyWindowProgress(0);

  TEST_ASSERT_EQUAL_INT(0, greenhouse.WindowProgress());
}

void Test_ApplyWindowProgress_CloseWithOpenDayMinimumInDay_PartlyClosed()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(20);
  greenhouse.OpenDayMinimum(10);
  greenhouse.DayStartHour(8);
  greenhouse.DayEndHour(20);
  greenhouse.m_mock_CurrentHour = 12; // 12pm

  greenhouse.ApplyWindowProgress(0);

  TEST_ASSERT_EQUAL_INT(10, greenhouse.WindowProgress());
}

void Test_ApplyWindowProgress_OpenDayMinimumAboveProgress_PartlyOpened()
{
  GreenhouseTest greenhouse;
  greenhouse.WindowProgress(5);
  greenhouse.OpenDayMinimum(10);
  greenhouse.DayStartHour(8);
  greenhouse.DayEndHour(20);
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

void Test_UpdateHeatingSystems_DaytimeBelowDayTemp_SwitchOnWaterHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 3;
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.DayWaterTemperature(2);
  greenhouse.DaySoilTemperature(2);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterHeating);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchWaterHeating_on);
}

void Test_UpdateHeatingSystems_DaytimeAboveDayTemp_SwitchOffWaterHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 3;
  greenhouse.m_mock_WaterTemperature = 3; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.DayWaterTemperature(1);
  greenhouse.WaterHeatingIsOn(true);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchWaterHeating_on);
}

void Test_UpdateHeatingSystems_BeforeDaytimeBelowNightTemp_SwitchOnWaterHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 1;
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightWaterTemperature(2);
  greenhouse.NightSoilTemperature(2);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterHeating);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchWaterHeating_on);
}

void Test_UpdateHeatingSystems_BeforeDaytimeAboveNightTemp_SwitchOffWaterHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 1;
  greenhouse.m_mock_WaterTemperature = 3; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightWaterTemperature(1);
  greenhouse.WaterHeatingIsOn(true);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchWaterHeating_on);
}

void Test_UpdateHeatingSystems_AfterDaytimeBelowNightTemp_SwitchOnWaterHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 4;
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightWaterTemperature(2);
  greenhouse.NightSoilTemperature(2);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterHeating);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchWaterHeating_on);
}

void Test_UpdateHeatingSystems_AfterDaytimeAboveNightTemp_SwitchOffWaterHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 4;
  greenhouse.m_mock_WaterTemperature = 3; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightWaterTemperature(1);
  greenhouse.WaterHeatingIsOn(true);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchWaterHeating_on);
}

void Test_UpdateHeatingSystems_DaytimeBelowDayTemp_SwitchOnSoilHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 3;
  greenhouse.m_mock_SoilTemperature = 0; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.DaySoilTemperature(2);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchSoilHeating);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchSoilHeating_on);
}

void Test_UpdateHeatingSystems_DaytimeAboveDayTemp_SwitchOffSoilHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 3;
  greenhouse.m_mock_SoilTemperature = 3; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.DaySoilTemperature(1);
  greenhouse.SoilHeatingIsOn(true);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchSoilHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchSoilHeating_on);
}

void Test_UpdateHeatingSystems_BeforeDaytimeBelowNightTemp_SwitchOnSoilHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 1;
  greenhouse.m_mock_SoilTemperature = 0; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightSoilTemperature(2);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchSoilHeating);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchSoilHeating_on);
}

void Test_UpdateHeatingSystems_BeforeDaytimeAboveNightTemp_SwitchOffSoilHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 1;
  greenhouse.m_mock_SoilTemperature = 3; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightSoilTemperature(1);
  greenhouse.SoilHeatingIsOn(true);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchSoilHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchSoilHeating_on);
}

void Test_UpdateHeatingSystems_AfterDaytimeBelowNightTemp_SwitchOnSoilHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 4;
  greenhouse.m_mock_SoilTemperature = 0; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightSoilTemperature(2);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchSoilHeating);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchSoilHeating_on);
}

void Test_UpdateHeatingSystems_AfterDaytimeAboveNightTemp_SwitchOffSoilHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 4;
  greenhouse.m_mock_SoilTemperature = 3; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightSoilTemperature(1);
  greenhouse.SoilHeatingIsOn(true);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchSoilHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchSoilHeating_on);
}

void Test_UpdateHeatingSystems_DaytimeBelowDayTemp_SwitchOnAirHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 3;
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.DayAirTemperature(2);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchAirHeating);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchAirHeating_on);
}

void Test_UpdateHeatingSystems_DaytimeAboveDayTemp_SwitchOffAirHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 3;
  greenhouse.m_mock_InsideAirTemperature = 3; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.DayAirTemperature(1);
  greenhouse.AirHeatingIsOn(true);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchAirHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchAirHeating_on);
}

void Test_UpdateHeatingSystems_BeforeDaytimeBelowNightTemp_SwitchOnAirHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 1;
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightAirTemperature(2);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchAirHeating);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchAirHeating_on);
}

void Test_UpdateHeatingSystems_BeforeDaytimeAboveNightTemp_SwitchOffAirHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 1;
  greenhouse.m_mock_InsideAirTemperature = 3; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightAirTemperature(1);
  greenhouse.AirHeatingIsOn(true);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchAirHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchAirHeating_on);
}

void Test_UpdateHeatingSystems_AfterDaytimeBelowNightTemp_SwitchOnAirHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 4;
  greenhouse.m_mock_InsideAirTemperature = 0; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightAirTemperature(2);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchAirHeating);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchAirHeating_on);
}

void Test_UpdateHeatingSystems_AfterDaytimeAboveNightTemp_SwitchOffAirHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 4;
  greenhouse.m_mock_InsideAirTemperature = 3; // remember margin
  greenhouse.m_mock_WaterTemperature = 0; // remember margin
  greenhouse.m_mock_SoilTemperature = 0; // remember margin

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.NightAirTemperature(1);
  greenhouse.AirHeatingIsOn(true);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchAirHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchAirHeating_on);
}

void Test_UpdateHeatingSystems_SoilBelowTempAndWaterBelowTempThenWaterAboveTemp_SwitchOffWaterHeatingAfterSwitchedOn(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 3;
  greenhouse.m_mock_SoilTemperature = 0;
  greenhouse.m_mock_WaterTemperature = 0;
  greenhouse.m_mock_InsideAirTemperature = 2;

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.DayWaterTemperature(2);
  greenhouse.DaySoilTemperature(2);
  greenhouse.DayAirTemperature(2);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchSoilHeating);
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterHeating);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchSoilHeating_on);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchWaterHeating_on);
  
  greenhouse.m_mock_SoilTemperature = 2; // on target, but not above/below margin
  greenhouse.m_mock_WaterTemperature = 4;
  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchSoilHeating);
  TEST_ASSERT_EQUAL_INT(2, greenhouse.m_calls_SwitchWaterHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchWaterHeating_on);
}

void Test_UpdateHeatingSystems_WaterOnWhenAirAndSoilHeatingOff_SwitchOffWaterHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 3;
  greenhouse.m_mock_SoilTemperature = 3;
  greenhouse.m_mock_WaterTemperature = 0;
  greenhouse.m_mock_InsideAirTemperature = 3;

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.DayWaterTemperature(2);
  greenhouse.DaySoilTemperature(2);
  greenhouse.DayAirTemperature(2);
  greenhouse.WaterHeatingIsOn(true);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchWaterHeating_on);
}

void Test_Refresh_RainDetectedInAutoMode_WindowClosed(void)
{
  GreenhouseTest greenhouse;
  greenhouse.AutoMode(true);
  greenhouse.WindowProgress(50);
  greenhouse.WeatherCode(700);

  greenhouse.Refresh();
  
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL(.5, greenhouse.m_lastArg_CloseWindow_delta);
}

void Test_Refresh_RainDetectedInManualMode_WindowClosed(void)
{
  GreenhouseTest greenhouse;
  greenhouse.AutoMode(false);
  greenhouse.WindowProgress(50);
  greenhouse.WeatherCode(700);

  greenhouse.Refresh();
  
  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_CloseWindow);
  TEST_ASSERT_EQUAL(.5, greenhouse.m_lastArg_CloseWindow_delta);
}

void Test_UpdateHeatingSystems_WaterHeaterRuntimeLimitReached_SwitchOffWaterHeating(void)
{
  GreenhouseTest greenhouse;

  greenhouse.m_mock_CurrentHour = 3;
  greenhouse.m_mock_SoilTemperature = 0;
  greenhouse.m_mock_WaterTemperature = 0;
  greenhouse.m_mock_InsideAirTemperature = 0;
  greenhouse.m_mock_UptimeSeconds = 0;

  greenhouse.DayStartHour(2);
  greenhouse.DayEndHour(4);
  greenhouse.DayWaterTemperature(2);
  greenhouse.DaySoilTemperature(2);
  greenhouse.DayAirTemperature(2);
  greenhouse.WaterHeaterLimitMinutes(1);

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(1, greenhouse.m_calls_SwitchWaterHeating);
  TEST_ASSERT_EQUAL(true, greenhouse.m_lastArg_SwitchWaterHeating_on);

  greenhouse.m_mock_UptimeSeconds = 60;

  greenhouse.UpdateHeatingSystems();

  TEST_ASSERT_EQUAL_INT(2, greenhouse.m_calls_SwitchWaterHeating);
  TEST_ASSERT_EQUAL(false, greenhouse.m_lastArg_SwitchWaterHeating_on);
  
  greenhouse.m_mock_UptimeSeconds = 120;

  greenhouse.UpdateHeatingSystems();

  // ensure water heating switch on is blocked
  TEST_ASSERT_EQUAL_INT(2, greenhouse.m_calls_SwitchWaterHeating);
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
  RUN_TEST(Test_OpenWindow_HalfDelta_ActuatorMovedForwardHalf);
  RUN_TEST(Test_CloseWindow_HalfDelta_ActuatorMovedBackwardHalf);
  RUN_TEST(Test_ApplyWindowProgress_CloseNoOpenDayMinimumInDay_FullyClosed);
  RUN_TEST(Test_ApplyWindowProgress_CloseWithOpenDayMinimumAtNight_FullyClosed);
  RUN_TEST(Test_ApplyWindowProgress_CloseWithOpenDayMinimumInDay_PartlyClosed);
  RUN_TEST(Test_ApplyWindowProgress_OpenDayMinimumAboveProgress_PartlyOpened);
  RUN_TEST(Test_CalculateMoisture_BelowOrEqualMin_ReturnsZero);
  RUN_TEST(Test_CalculateMoisture_AboveOrEqualMax_ReturnsHundred);
  RUN_TEST(Test_CalculateMoisture_InBounds_ReturnsPercent);
  RUN_TEST(Test_UpdateHeatingSystems_DaytimeBelowDayTemp_SwitchOnWaterHeating);
  RUN_TEST(Test_UpdateHeatingSystems_DaytimeAboveDayTemp_SwitchOffWaterHeating);
  RUN_TEST(Test_UpdateHeatingSystems_BeforeDaytimeBelowNightTemp_SwitchOnWaterHeating);
  RUN_TEST(Test_UpdateHeatingSystems_BeforeDaytimeAboveNightTemp_SwitchOffWaterHeating);
  RUN_TEST(Test_UpdateHeatingSystems_AfterDaytimeBelowNightTemp_SwitchOnWaterHeating);
  RUN_TEST(Test_UpdateHeatingSystems_AfterDaytimeAboveNightTemp_SwitchOffWaterHeating);
  RUN_TEST(Test_UpdateHeatingSystems_DaytimeBelowDayTemp_SwitchOnSoilHeating);
  RUN_TEST(Test_UpdateHeatingSystems_DaytimeAboveDayTemp_SwitchOffSoilHeating);
  RUN_TEST(Test_UpdateHeatingSystems_BeforeDaytimeBelowNightTemp_SwitchOnSoilHeating);
  RUN_TEST(Test_UpdateHeatingSystems_BeforeDaytimeAboveNightTemp_SwitchOffSoilHeating);
  RUN_TEST(Test_UpdateHeatingSystems_AfterDaytimeBelowNightTemp_SwitchOnSoilHeating);
  RUN_TEST(Test_UpdateHeatingSystems_AfterDaytimeAboveNightTemp_SwitchOffSoilHeating);
  RUN_TEST(Test_UpdateHeatingSystems_DaytimeBelowDayTemp_SwitchOnAirHeating);
  RUN_TEST(Test_UpdateHeatingSystems_DaytimeAboveDayTemp_SwitchOffAirHeating);
  RUN_TEST(Test_UpdateHeatingSystems_BeforeDaytimeBelowNightTemp_SwitchOnAirHeating);
  RUN_TEST(Test_UpdateHeatingSystems_BeforeDaytimeAboveNightTemp_SwitchOffAirHeating);
  RUN_TEST(Test_UpdateHeatingSystems_AfterDaytimeBelowNightTemp_SwitchOnAirHeating);
  RUN_TEST(Test_UpdateHeatingSystems_AfterDaytimeAboveNightTemp_SwitchOffAirHeating);
  RUN_TEST(Test_UpdateHeatingSystems_SoilBelowTempAndWaterBelowTempThenWaterAboveTemp_SwitchOffWaterHeatingAfterSwitchedOn);
  RUN_TEST(Test_UpdateHeatingSystems_WaterOnWhenAirAndSoilHeatingOff_SwitchOffWaterHeating);
  RUN_TEST(Test_Refresh_RainDetectedInAutoMode_WindowClosed);
  RUN_TEST(Test_Refresh_RainDetectedInManualMode_WindowClosed);
  RUN_TEST(Test_UpdateHeatingSystems_WaterHeaterRuntimeLimitReached_SwitchOffWaterHeating);
}
