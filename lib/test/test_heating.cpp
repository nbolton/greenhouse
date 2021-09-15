#include "test_heating.h"

#include "../native/greenhouse/Heating.h"

#include "TestSystem.h"
#include <unity.h>

using namespace native::greenhouse;

class TestHeating : public Heating {
};

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void Test_Update_DaytimeBelowDayTemp_SwitchOnWaterHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.DayWaterTemperature(2);
  heating.DaySoilTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.WaterHeatingIsOn());
}

void Test_Update_DaytimeAboveDayTemp_SwitchOffWaterHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_WaterTemperature = 3;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.DayWaterTemperature(1);
  heating.WaterHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeatingIsOn());
}

void Test_Update_BeforeDaytimeBelowNightTemp_SwitchOnWaterHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightWaterTemperature(2);
  heating.NightSoilTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.WaterHeatingIsOn());
}

void Test_Update_BeforeDaytimeAboveNightTemp_SwitchOffWaterHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_WaterTemperature = 3;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightWaterTemperature(1);
  heating.WaterHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeatingIsOn());
}

void Test_Update_AfterDaytimeBelowNightTemp_SwitchOnWaterHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightWaterTemperature(2);
  heating.NightSoilTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.WaterHeatingIsOn());
}

void Test_Update_AfterDaytimeAboveNightTemp_SwitchOffWaterHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_WaterTemperature = 3;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightWaterTemperature(1);
  heating.WaterHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeatingIsOn());
}

void Test_Update_DaytimeBelowDayTemp_SwitchOnSoilHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.DaySoilTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.SoilHeatingIsOn());
}

void Test_Update_DaytimeAboveDayTemp_SwitchOffSoilHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_SoilTemperature = 3;      // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin + min water delta
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.DaySoilTemperature(1);
  heating.SoilHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.SoilHeatingIsOn());
}

void Test_Update_BeforeDaytimeBelowNightTemp_SwitchOnSoilHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin + min water delta
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightSoilTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.SoilHeatingIsOn());
}

void Test_Update_BeforeDaytimeAboveNightTemp_SwitchOffSoilHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_SoilTemperature = 3;      // remember margin
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightSoilTemperature(1);
  heating.SoilHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.SoilHeatingIsOn());
}

void Test_Update_AfterDaytimeBelowNightTemp_SwitchOnSoilHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin + min water delta
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightSoilTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.SoilHeatingIsOn());
}

void Test_Update_AfterDaytimeAboveNightTemp_SwitchOffSoilHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_SoilTemperature = 3;      // remember margin
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightSoilTemperature(1);
  heating.SoilHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.SoilHeatingIsOn());
}

void Test_Update_DaytimeBelowDayTemp_SwitchOnAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_InsideAirTemperature = 0; // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin + min water delta
  system.m_mock_SoilTemperature = 0;      // remember margin

  heating.DayAirTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.AirHeatingIsOn());
}

void Test_Update_DaytimeAboveDayTemp_SwitchOffAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_InsideAirTemperature = 3; // remember margin
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin

  heating.DayAirTemperature(1);
  heating.AirHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.AirHeatingIsOn());
}

void Test_Update_BeforeDaytimeBelowNightTemp_SwitchOnAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_InsideAirTemperature = 0; // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin + min water delta
  system.m_mock_SoilTemperature = 0;      // remember margin

  heating.NightAirTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.AirHeatingIsOn());
}

void Test_Update_BeforeDaytimeAboveNightTemp_SwitchOffAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_InsideAirTemperature = 3; // remember margin
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin

  heating.NightAirTemperature(1);
  heating.AirHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.AirHeatingIsOn());
}

void Test_Update_AfterDaytimeBelowNightTemp_SwitchOnAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_InsideAirTemperature = 0; // remember margin
  system.m_mock_WaterTemperature = 5;     // remember margin + min water delta
  system.m_mock_SoilTemperature = 0;      // remember margin

  heating.NightAirTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.AirHeatingIsOn());
}

void Test_Update_AfterDaytimeAboveNightTemp_SwitchOffAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_InsideAirTemperature = 3; // remember margin
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin

  heating.NightAirTemperature(1);
  heating.AirHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.AirHeatingIsOn());
}

void Test_Update_SoilBelowTempAndWaterBelowTempThenWaterAboveTemp_SwitchOffWaterHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_SoilTemperature = 0;
  system.m_mock_WaterTemperature = 5; // remember margin + min water delta
  system.m_mock_InsideAirTemperature = 2;

  heating.DayWaterTemperature(7);
  heating.DaySoilTemperature(2);
  heating.DayAirTemperature(2);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.SoilHeatingIsOn());
  TEST_ASSERT_EQUAL(true, heating.WaterHeatingIsOn());

  system.m_mock_SoilTemperature = 2; // on target, but not above/below margin
  system.m_mock_WaterTemperature = 6;
  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeatingIsOn());
}

void Test_Update_WaterOnWhenAirAndSoilHeatingOff_SwitchOffWaterHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_SoilTemperature = 3;
  system.m_mock_WaterTemperature = 0;
  system.m_mock_InsideAirTemperature = 3;

  heating.DayWaterTemperature(2);
  heating.DaySoilTemperature(2);
  heating.DayAirTemperature(2);
  heating.WaterHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeatingIsOn());
}

void Test_Update_WaterHeaterRuntimeLimitReached_SwitchOffWaterHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_SoilTemperature = 0;
  system.m_mock_WaterTemperature = 0;
  system.m_mock_InsideAirTemperature = 0;
  system.m_mock_UptimeSeconds = 0;

  heating.DayWaterTemperature(8);
  heating.DaySoilTemperature(2);
  heating.DayAirTemperature(2);
  heating.WaterHeaterLimitMinutes(1);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.WaterHeatingIsOn());

  system.m_mock_UptimeSeconds = 60;

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeatingIsOn());

  system.m_mock_UptimeSeconds = 120;

  heating.Update();

  // ensure water heating switch on is blocked
  TEST_ASSERT_EQUAL(false, heating.WaterHeatingIsOn());
}

void Test_Update_DaytimeTransition_WaterHeaterRuntimeLimitIsReset(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_SoilTemperature = 0;
  system.m_mock_WaterTemperature = 0;
  system.m_mock_InsideAirTemperature = 0;
  system.m_mock_UptimeSeconds = 0;

  heating.DayWaterTemperature(2);
  heating.DaySoilTemperature(2);
  heating.DayAirTemperature(2);
  heating.WaterHeaterLimitMinutes(1);

  heating.Update();

  TEST_ASSERT_EQUAL_INT(0, heating.WaterHeatingRuntimeMinutes());

  system.m_mock_UptimeSeconds = 60;
  system.m_mock_IsDaytime = false;

  heating.Update();

  TEST_ASSERT_FLOAT_WITHIN(.1f, 1, heating.WaterHeatingRuntimeMinutes());

  system.m_mock_UptimeSeconds = 3;
  system.m_mock_IsDaytime = true;

  heating.Update();

  TEST_ASSERT_EQUAL_INT(0, heating.WaterHeatingRuntimeMinutes());
}

void Test_Update_DaytimeBelowDayTempAndWaterNotWarmEnough_SwitchOffSoilAndAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.DayWaterTemperature(2);
  heating.DaySoilTemperature(2);
  heating.DayAirTemperature(2);
  heating.WaterHeatingIsOn(false);
  heating.SoilHeatingIsOn(true);
  heating.AirHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.WaterHeatingIsOn());
  TEST_ASSERT_EQUAL(false, heating.SoilHeatingIsOn());
  TEST_ASSERT_EQUAL(false, heating.AirHeatingIsOn());
}

void Test_Update_BeforeDaytimeBelowDayTempAndWaterNotWarmEnough_SwitchOffSoilAndAirHeating(void)
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = false;
  system.m_mock_WaterTemperature = 0;     // remember margin
  system.m_mock_SoilTemperature = 0;      // remember margin
  system.m_mock_InsideAirTemperature = 0; // remember margin

  heating.NightWaterTemperature(2);
  heating.NightSoilTemperature(2);
  heating.NightAirTemperature(2);
  heating.WaterHeatingIsOn(false);
  heating.SoilHeatingIsOn(true);
  heating.AirHeatingIsOn(true);

  heating.Update();

  TEST_ASSERT_EQUAL(true, heating.WaterHeatingIsOn());
  TEST_ASSERT_EQUAL(false, heating.SoilHeatingIsOn());
  TEST_ASSERT_EQUAL(false, heating.AirHeatingIsOn());
}

void Test_Update_DaytimeAirBelowTemp_WaterTooLowAndWaterLimitReached_SwitchOffAirHeating()
{
  TestSystem system;
  TestHeating heating;
  heating.System(system);

  system.m_mockOn_IsDaytime = true;
  system.m_mock_IsDaytime = true;
  system.m_mock_WaterTemperature = 21.94;     // remember margin
  system.m_mock_SoilTemperature = 21.69;      // remember margin
  system.m_mock_InsideAirTemperature = 20.04; // remember margin

  heating.DayWaterTemperature(40);
  heating.DaySoilTemperature(24);
  heating.DayAirTemperature(21);
  heating.WaterHeatingIsOn(false);
  heating.SoilHeatingIsOn(false);
  heating.AirHeatingIsOn(true);
  heating.WaterHeaterLimitMinutes(1);
  heating.WaterHeatingRuntimeMinutes(1);

  heating.Update();

  TEST_ASSERT_EQUAL(false, heating.WaterHeatingIsOn());
  TEST_ASSERT_EQUAL(false, heating.SoilHeatingIsOn());
  TEST_ASSERT_EQUAL(false, heating.AirHeatingIsOn());
}

void testHeating()
{
  RUN_TEST(Test_Update_DaytimeBelowDayTemp_SwitchOnWaterHeating);
  RUN_TEST(Test_Update_DaytimeAboveDayTemp_SwitchOffWaterHeating);
  RUN_TEST(Test_Update_BeforeDaytimeBelowNightTemp_SwitchOnWaterHeating);
  RUN_TEST(Test_Update_BeforeDaytimeAboveNightTemp_SwitchOffWaterHeating);
  RUN_TEST(Test_Update_AfterDaytimeBelowNightTemp_SwitchOnWaterHeating);
  RUN_TEST(Test_Update_AfterDaytimeAboveNightTemp_SwitchOffWaterHeating);
  RUN_TEST(Test_Update_DaytimeBelowDayTemp_SwitchOnSoilHeating);
  RUN_TEST(Test_Update_DaytimeAboveDayTemp_SwitchOffSoilHeating);
  RUN_TEST(Test_Update_BeforeDaytimeBelowNightTemp_SwitchOnSoilHeating);
  RUN_TEST(Test_Update_BeforeDaytimeAboveNightTemp_SwitchOffSoilHeating);
  RUN_TEST(Test_Update_AfterDaytimeBelowNightTemp_SwitchOnSoilHeating);
  RUN_TEST(Test_Update_AfterDaytimeAboveNightTemp_SwitchOffSoilHeating);
  RUN_TEST(Test_Update_DaytimeBelowDayTemp_SwitchOnAirHeating);
  RUN_TEST(Test_Update_DaytimeAboveDayTemp_SwitchOffAirHeating);
  RUN_TEST(Test_Update_BeforeDaytimeBelowNightTemp_SwitchOnAirHeating);
  RUN_TEST(Test_Update_BeforeDaytimeAboveNightTemp_SwitchOffAirHeating);
  RUN_TEST(Test_Update_AfterDaytimeBelowNightTemp_SwitchOnAirHeating);
  RUN_TEST(Test_Update_AfterDaytimeAboveNightTemp_SwitchOffAirHeating);
  RUN_TEST(Test_Update_SoilBelowTempAndWaterBelowTempThenWaterAboveTemp_SwitchOffWaterHeating);
  RUN_TEST(Test_Update_WaterOnWhenAirAndSoilHeatingOff_SwitchOffWaterHeating);
  RUN_TEST(Test_Update_WaterHeaterRuntimeLimitReached_SwitchOffWaterHeating);
  RUN_TEST(Test_Update_DaytimeTransition_WaterHeaterRuntimeLimitIsReset);
  RUN_TEST(Test_Update_DaytimeBelowDayTempAndWaterNotWarmEnough_SwitchOffSoilAndAirHeating);
  RUN_TEST(Test_Update_BeforeDaytimeBelowDayTempAndWaterNotWarmEnough_SwitchOffSoilAndAirHeating);
  RUN_TEST(Test_Update_DaytimeAirBelowTemp_WaterTooLowAndWaterLimitReached_SwitchOffAirHeating);
}
